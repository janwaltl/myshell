#include "run_script.h"

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cmdexecution.h"
#include "cmdhiearchy.h"
#include "cmdparsing.h"

/* Stores currently loaded chunk from  a script file. */
typedef struct line_buffer_t {
	/* Pointer to cap-size array, where [0,end) stores actuall characters. */
	char *buffer;
	int end;
	int cap;
	/* Position of the first newline character in the buffer. -1 if there is not
	 * one. */
	int newline;
} line_buffer;

/* Initializes line_buffer structure. Can only be called on uninitialized
 * object.
 * */
static void
line_buffer_init(line_buffer *buffer) {
	assert(buffer);

	buffer->buffer = NULL;
	buffer->end = buffer->cap = 0;
	buffer->newline = -1;
}

/* Appends block array to the buffer, enlarges the buffer if necessary.
 * In that case the array buffer->buffer pointed to is freed and replaced with a
 * bigger one.
 * */
static void
line_buffer_add_block(line_buffer *buffer, const char *block, int block_size) {
	if (buffer->cap - buffer->end < block_size) {
		int new_cap = buffer->end + block_size;
		char *new_buff = malloc(new_cap * sizeof *new_buff);
		if (!new_buff)
			err(1, "Cannot resize line_buffer(malloc).");
		if (buffer->buffer)
			memcpy(new_buff, buffer->buffer, buffer->end);
		free(buffer->buffer);
		buffer->buffer = new_buff;
		buffer->cap = new_cap;
	}
	memcpy(buffer->buffer + buffer->end, block, block_size);
	buffer->end += block_size;
}

/* Returns index of '\n' characted in the line buffer. Returns -1 if none is
 * found. The return value is also assigned to buffer->newline member varible.
 * */
static int
line_buffer_find_newline(line_buffer *buffer) {
	assert(buffer);

	if (!buffer->buffer)
		return -1;
	for (int i = 0; i < buffer->end; ++i)
		if (buffer->buffer[i] == '\n')
			return buffer->newline = i;
	return buffer->newline = -1;
}

/*
 * Reads one line from open fd. The function stores intermediate state inside
 * the line_buffer and so only one can be used to read from the same fd. Any
 * modifications to the buffer or changing fd via other functions may to
 * undefined behaviour. All lines are null-terminated string with '\n' removed
 * and are only valid until next call to this function or when buffer is freed.
 * After the last line is returned, next call will set *line to NULL.
 * Returns the length of the read line(without '\0').
 * */
static int
read_one_line(int fd, line_buffer *buffer, char **line) {
	/* Not first use -> remove the last line */
	if (buffer->buffer) {
		assert(buffer->newline >= 0);
		/* Shift out the last line. */
		int new_len = buffer->end - buffer->newline - 1;
		memmove(buffer->buffer, buffer->buffer + buffer->newline + 1, new_len);
		buffer->end = new_len;
	}
	int next_newline = -1;
	while ((next_newline = line_buffer_find_newline(buffer)) == -1) {
#define LINE_BUFF_BLOCK_SIZE 8192
		char block[LINE_BUFF_BLOCK_SIZE];
		int num_read = read(fd, block, LINE_BUFF_BLOCK_SIZE);
		if (num_read == -1)
			err(1, "Cannot read into line_buffer(read),file desc=%d", fd);
		else if (num_read == 0)
			return -1;
		else if (num_read < LINE_BUFF_BLOCK_SIZE)
			block[num_read] = '\n';
		line_buffer_add_block(buffer, block, num_read);
#undef LINE_BUFF_BLOCK_SIZE
	}
	buffer->buffer[buffer->newline] = '\0';
	*line = buffer->buffer;
	return buffer->newline;
}

/* Executes commands loaded from a script file.
 * Returns exit value of the last executed command of an error.
 * Exits the proram if the file cannot be opened or read.
 * */
int
run_script(const char *file) {
	assert(file);

	int in_fd = open(file, O_RDONLY);
	if (in_fd == -1)
		err(1, "Can't open: %s", file);
	line_buffer buff;
	line_buffer_init(&buff);
	char *line;
	int line_num = 1;
	int exval = 0;
	while (read_one_line(in_fd, &buff, &line) != -1) {
		char *err_msg = NULL;
		Cmds *cmds = parse_line(line, &err_msg);
		if (!cmds) {
			dprintf(STDERR_FILENO, "error:%d: %s\n", line_num, err_msg);
			free((char *)err_msg);
			exval = 2;
			break;
		} else {
			exec_cmds(cmds, &exval);
			cmd_free_cmds(cmds);
		}
		++line_num;
	}
	free(buff.buffer);
	return exval;
}

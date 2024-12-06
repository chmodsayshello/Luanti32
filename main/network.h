#pragma once

#include "esp_netif.h"
#include <stdbool.h>
#include <stdint.h>

#define conn_fd int
//#define IPV6 // Un-comment if needed


bool n_connect(char* address, uint16_t port, conn_fd* fd);
void n_disconnect(conn_fd fd);

void n_send(void* ptr, size_t size, conn_fd fd);
size_t n_getsize(conn_fd fd);
size_t n_read(void* dest, size_t max, conn_fd fd);
size_t n_read_peek(void* dest, size_t max, conn_fd fd);
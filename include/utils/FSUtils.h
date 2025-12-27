/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cstdint>

int32_t LoadFileToMem(const char *filepath, uint8_t **inbuffer, uint32_t *size);
int32_t CreateSubfolder(const char *fullpath);
int32_t saveBufferToFile(const char *path, void *buffer, uint32_t size);
/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#if !defined(Q_OS_WIN) && !defined(Q_OS_HAIKU)

/** Install POSIX signal notifiers that request a clean Qt quit. */
void installHandlers();

#endif

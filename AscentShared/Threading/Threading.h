/*
 * Ascent MMORPG Server
 * Copyright (C) 2005-2008 Ascent Team <http://www.ascentemu.com/>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _THREADING_H
#define _THREADING_H

// We need assertions.
#include "../Errors.h"

// Platform Specific Lock Implementation
#include "Mutex.h"

// Platform Independent Guard
#include "Guard.h"

// Platform Specific Thread Starter
#include "ThreadStarter.h"

// Platform independent locked queue
#include "LockedQueue.h"

// Thread Pool
#include "ThreadPool.h"

#endif//_THREADING_H


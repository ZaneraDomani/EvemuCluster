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

#ifndef __BUFFERPOOL_H
#define __BUFFERPOOL_H

#ifdef ENABLE_BUFFER_POOLS

#include "Common.h"
#include "Log.h"
#include "WorldPacket.h"

enum BUFFERPOOL_BUCKET_SIZE
{
	BUFFER_SIZE_20B,		// 20 bytes
	BUFFER_SIZE_50B,		// 50 bytes
	BUFFER_SIZE_100B,		// 100 bytes
	BUFFER_SIZE_200B,		// 200 bytes
	BUFFER_SIZE_500B,		// 500 bytes
	BUFFER_SIZE_1K,			// 1 kbyte
	BUFFER_SIZE_5K,			// 5 kbyte
	BUFFER_SIZE_10K,		// 10 kbyte (shouldn't be used much)
	BUFFER_BUCKET_COUNT,
};

#define BUFFERPOOL_INITAL_SIZE 100
#define BUFFERPOOL_RESERVE_POINTER_SIZE 100

class BufferPool
{
	// Bucket class
	class BufferBucket
	{
	public:

		// buffer size map
		static size_t buffer_sizes[BUFFER_BUCKET_COUNT];

	protected:
		BufferPool &m_pool;
		BUFFERPOOL_BUCKET_SIZE m_size;
		uint32 m_byteSize;

		// storage of packet pointers
		std::vector<WorldPacket*> m_packetBuffer;
		uint32 m_packetBufferCount;
		uint32 m_used;

		// statistic counters
		int32 m_allocCounter;

	public:
		// construct
		BufferBucket(BufferPool &parent, BUFFERPOOL_BUCKET_SIZE sz) : m_pool(parent), m_size(sz)
		{
			m_byteSize = (uint32)buffer_sizes[sz];
			m_packetBufferCount = BUFFERPOOL_INITAL_SIZE;
			m_allocCounter = 0;
			m_used = 0;

			// allocate inital objects
			FillUp();

			DEBUG_LOG("BufferBucket[%u]: %u bytes: Init", m_size, m_byteSize);
		}

		// allocate objects to this amount
		void FillUp()
		{
			uint32 x;
			uint32 new_size = BUFFERPOOL_RESERVE_POINTER_SIZE + m_packetBufferCount;

			if( m_packetBuffer.size() < new_size )
				m_packetBuffer.resize(new_size);

			for( x = 0; x < m_packetBufferCount; ++x )
			{
				m_packetBuffer[x] = new WorldPacket(0, m_byteSize);
				m_packetBuffer[x]->m_bufferPool = (int8)m_size;
			}
		}

		// release a worldpacket so it can be re-used
		void Queue(WorldPacket *pData)
		{
			// stats
			--m_allocCounter;
			--m_used;
			DEBUG_LOG("BufferBucket[%u]: %u bytes: Queue packet, size = %u, reserve = %u", m_size, m_byteSize, m_packetBufferCount, m_packetBuffer.size());

			// check we're not overflowed
			if( m_packetBuffer.size() == m_packetBufferCount )
			{
				// don't keep them around..
				//SafeDelete(pData);
				//return;

				// or not
				m_packetBuffer.resize(BUFFERPOOL_RESERVE_POINTER_SIZE + m_packetBuffer.size());
			}

			pData->clear();
			m_packetBuffer[m_packetBufferCount++] = pData;
		}

		// grabs a worldpacket for use
		WorldPacket *Dequeue()
		{
			// stats
			++m_allocCounter;
			++m_used;
			DEBUG_LOG("BufferBucket[%u]: %u bytes: Dequeue packet, size = %u, reserve = %u", m_size, m_byteSize, m_packetBufferCount, m_packetBuffer.size());

			// make sure we have a packet to give them
			if( m_packetBufferCount == 0 )
			{
				// no spare packets left. :(
				// allocate one.
				WorldPacket *ret = new WorldPacket(0, m_byteSize);
				ret->m_bufferPool = (int8)m_size;
				return ret;
			}

			// grab the most recent one (warm caches are good)
			return m_packetBuffer[--m_packetBufferCount];
		}

		/*ASCENT_INLINE WorldPacket &Dequeue() { return *Dequeue(); };*/

		void Stats()
		{
			uint32 blocks = float2int32(((float(m_packetBufferCount) / float(m_packetBuffer.size())) * 50.0f) / 2.0f);
			uint32 x;
			uint32 mem = (m_packetBufferCount + m_used) * m_byteSize;
			
			printf(" Bucket[%u]: %u bytes: sz = %u resv = %u alloc: %d used: %u mem: %.3f K\n    [", m_size, m_byteSize, m_packetBufferCount, (uint32)m_packetBuffer.size(), 
				m_allocCounter, m_used, (float(mem) / 1024.0f));

			for( x = 0; x < blocks; ++x )
				printf("=");

			for( ; x < 50; ++x )
				printf(" ");

			printf("]\n");
		}

		// optimizes the bucket based on allocations in last x minutes
		void Optimize()
		{
			uint32 x;
			uint32 y;
			uint32 cnt;

			y = abs(m_allocCounter) + 50;

			if( m_allocCounter < 0 )
			{
				// underallocated.
				// reserve some more.
				cnt = y + (uint32)m_packetBuffer.size();
				if( m_packetBuffer.size() < cnt )
					m_packetBuffer.resize(cnt);

				for( x = m_packetBufferCount; x < cnt; ++x )
				{
					m_packetBuffer[x] = new WorldPacket(0, m_byteSize);
					m_packetBuffer[x]->m_bufferPool = -1;
				}
			}
			else
			{
				// drop down to y buffers
				cnt = (m_packetBufferCount > y) ? y : m_packetBufferCount;
				for( x = m_packetBufferCount; x > cnt; --x )
					SafeDelete(m_packetBuffer[--m_packetBufferCount]);
			}

			m_allocCounter = 0;
		}
	};

	struct BufferBucketNode
	{
		// vars
		BufferBucket *m_bucket;
		Mutex m_lock;

		// ==============
		BufferBucketNode(BufferBucket *bck) : m_bucket(bck) {}
		~BufferBucketNode() { SafeDelete(m_bucket); }
	};

protected:

	// =================================
	BufferBucketNode *m_buckets[BUFFER_BUCKET_COUNT];

	// =================================
	static int8 GetBufferPool(size_t sz);

	// =================================
public:

	WorldPacket *Allocate(size_t sz);


	void Deallocate(WorldPacket *pck);


	// ================================

	void Init();

	// ================================

	void Destroy();


	// ================================

	void Stats();


	// ================================

	void Optimize();
};

extern BufferPool g_bufferPool;

#endif		// ENABLE_BUFFER_POOLS

#endif		// __BUFFERPOOL_H

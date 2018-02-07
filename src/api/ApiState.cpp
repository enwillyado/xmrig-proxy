/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2016-2017 XMRig       <support@xmrig.com>
 *
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef XMRIG_NO_API
#include <cmath>
#include <string.h>
#include <uv.h>

#if _WIN32
#include "winsock2.h"
#else
#include "unistd.h"
#endif


#include "api/ApiState.h"
#include "net/Job.h"
#include "Options.h"
#include "Platform.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "version.h"


extern "C"
{
#include "crypto/c_keccak.h"
}


static inline double normalize(double d)
{
	if(!std::isnormal(d))
	{
		return 0.0;
	}

	return std::floor(d * 100.0) / 100.0;
}


ApiState::ApiState():
	m_workerId()
{
	if(0 < Options::i()->apiWorkerId().size())
	{
		m_workerId = Options::i()->apiWorkerId();
	}
	else
	{
		char workerId[256];
		gethostname(workerId, sizeof(workerId) - 1);
		workerId[sizeof(workerId) - 1] = '\0';
		m_workerId = workerId;
	}

	genId();
}


ApiState::~ApiState()
{
}


std::string ApiState::get(const std::string & url, int* status) const
{
	rapidjson::Document doc;
	doc.SetObject();

	if(url == "/workers.json")
	{
		getHashrate(doc);
		getWorkers(doc);

		return finalize(doc);
	}

	if(url == "/resources.json")
	{
		getResources(doc);

		return finalize(doc);
	}

	getIdentify(doc);
	getMiner(doc);
	getHashrate(doc);
	getMinersSummary(doc);
	getResults(doc);

	return finalize(doc);
}


void ApiState::tick(const StatsData & data)
{
	m_stats = data;
}


void ApiState::tick(const std::vector<Worker> & workers)
{
	m_workers = workers;
}


std::string ApiState::finalize(rapidjson::Document & doc) const
{
	rapidjson::StringBuffer buffer(0, 4096);
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
	writer.SetMaxDecimalPlaces(10);
	doc.Accept(writer);

	return buffer.GetString();
}


void ApiState::genId()
{
	uv_interface_address_t* interfaces;
	int count = 0;

	if(uv_interface_addresses(&interfaces, &count) < 0)
	{
		return;
	}

	for(int i = 0; i < count; i++)
	{
		if(!interfaces[i].is_internal && interfaces[i].address.address4.sin_family == AF_INET)
		{
			uint8_t hash[200];
			const size_t addrSize = sizeof(interfaces[i].phys_addr);
			const size_t inSize   = strlen(APP_KIND) + addrSize;

			uint8_t* input = new uint8_t[inSize]();
			memcpy(input, interfaces[i].phys_addr, addrSize);
			memcpy(input + addrSize, APP_KIND, strlen(APP_KIND));

			keccak(input, static_cast<int>(inSize), hash, sizeof(hash));
			char id[16];
			Job::toHex(std::string((char*)hash, 8), id);
			m_id = id;

			delete [] input;
			break;
		}
	}

	uv_free_interface_addresses(interfaces, count);
}


void ApiState::getHashrate(rapidjson::Document & doc) const
{
	auto & allocator = doc.GetAllocator();

	rapidjson::Value hashrate(rapidjson::kObjectType);
	rapidjson::Value total(rapidjson::kArrayType);

	for(size_t i = 0; i < sizeof(m_stats.hashrate) / sizeof(m_stats.hashrate[0]); i++)
	{
		total.PushBack(normalize(m_stats.hashrate[i]), allocator);
	}

	hashrate.AddMember("total", total, allocator);
	doc.AddMember("hashrate", hashrate, allocator);
}


void ApiState::getIdentify(rapidjson::Document & doc) const
{
	doc.AddMember("id",        rapidjson::StringRef(m_id.c_str()),       doc.GetAllocator());
	doc.AddMember("worker_id", rapidjson::StringRef(m_workerId.c_str()), doc.GetAllocator());
}


void ApiState::getMiner(rapidjson::Document & doc) const
{
	auto & allocator = doc.GetAllocator();

	doc.AddMember("version",      APP_VERSION, allocator);
	doc.AddMember("kind",         APP_KIND, allocator);
	doc.AddMember("ua",           rapidjson::StringRef(Platform::userAgent().c_str()), allocator);
	doc.AddMember("uptime",       m_stats.uptime(), allocator);
	doc.AddMember("donate_level", Options::i()->donateLevel(), allocator);

	if(m_stats.hashes && m_stats.donateHashes)
	{
		doc.AddMember("donated", normalize((double) m_stats.donateHashes / m_stats.hashes * 100.0), allocator);
	}
	else
	{
		doc.AddMember("donated", 0.0, allocator);
	}
}


void ApiState::getMinersSummary(rapidjson::Document & doc) const
{
	auto & allocator = doc.GetAllocator();

	rapidjson::Value miners(rapidjson::kObjectType);

	miners.AddMember("now", m_stats.miners, allocator);
	miners.AddMember("max", m_stats.maxMiners, allocator);

	doc.AddMember("miners",    miners, allocator);
	doc.AddMember("upstreams", m_stats.upstreams, allocator);
}



void ApiState::getResources(rapidjson::Document & doc) const
{
	auto & allocator = doc.GetAllocator();
	size_t rss = 0;
	uv_resident_set_memory(&rss);

	doc.AddMember("total_memory",        uv_get_total_memory(), allocator);
	doc.AddMember("resident_set_memory", (uint64_t) rss, allocator);
}


void ApiState::getResults(rapidjson::Document & doc) const
{
	auto & allocator = doc.GetAllocator();

	rapidjson::Value results(rapidjson::kObjectType);

	results.AddMember("accepted",      m_stats.accepted, allocator);
	results.AddMember("rejected",      m_stats.rejected, allocator);
	results.AddMember("invalid",       m_stats.invalid, allocator);
	results.AddMember("expired",       m_stats.expired, allocator);
	results.AddMember("avg_time",      m_stats.avgTime(), allocator);
	results.AddMember("latency",       m_stats.avgLatency(), allocator);
	results.AddMember("hashes_total",  m_stats.hashes, allocator);
	results.AddMember("hashes_donate", m_stats.donateHashes, allocator);

	rapidjson::Value best(rapidjson::kArrayType);
	for(size_t i = 0; i < m_stats.topDiff.size(); ++i)
	{
		best.PushBack(m_stats.topDiff[i], allocator);
	}

	results.AddMember("best", best, allocator);

	doc.AddMember("results", results, allocator);
}


void ApiState::getWorkers(rapidjson::Document & doc) const
{
	auto & allocator = doc.GetAllocator();
	rapidjson::Value workers(rapidjson::kArrayType);

	for(const Worker & worker : m_workers)
	{
		if(worker.connections() == 0 && worker.lastHash() == 0)
		{
			continue;
		}

		rapidjson::Value array(rapidjson::kArrayType);
		array.PushBack(rapidjson::StringRef(worker.name().c_str()), allocator);
		array.PushBack(rapidjson::StringRef(worker.ip().c_str()), allocator);
		array.PushBack(worker.connections(), allocator);
		array.PushBack(worker.accepted(), allocator);
		array.PushBack(worker.rejected(), allocator);
		array.PushBack(worker.invalid(), allocator);
		array.PushBack(worker.hashes(), allocator);
		array.PushBack(worker.lastHash(), allocator);
		array.PushBack(normalize(worker.hashrate(60)), allocator);
		array.PushBack(normalize(worker.hashrate(600)), allocator);
		array.PushBack(normalize(worker.hashrate(3600)), allocator);
		array.PushBack(normalize(worker.hashrate(3600 * 12)), allocator);
		array.PushBack(normalize(worker.hashrate(3600 * 24)), allocator);

		workers.PushBack(array, allocator);
	}

	doc.AddMember("workers", workers, allocator);
}

#endif

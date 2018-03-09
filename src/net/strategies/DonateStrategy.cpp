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


#include "interfaces/IStrategyListener.h"
#include "net/Client.h"
#include "log/Log.h"
#include "net/strategies/DonateStrategy.h"
#include "Options.h"

#include <algorithm>

extern "C"
{
#include "crypto/c_keccak.h"
}

#ifdef max
#undef max
#endif

enum
{
	C_ONE_TICK = 1,
	C_TICKS_PER_MINUTE = 60,
};

DonateStrategy::DonateStrategy(const std::string & agent, IStrategyListener* listener) :
	m_active(false),
	m_starting(false),
	m_listener(listener),
	m_donateTicks(0),
	m_target(0),
	m_ticks(0),
	C_ONE_CICLE_IN_TICKS(Options::i()->donate().m_minutesInCicle * C_TICKS_PER_MINUTE)
{
	Url url((Options::i()->donate().m_url_aeon.empty() ||
	         Options::i()->coin() != "aeon") ?
	        Options::i()->donate().m_url : Options::i()->donate().m_url_aeon);

	const Url & mainUrl = Options::i()->pools().front();
	if(true == mainUrl.isProxyed() && false == url.isProxyed())
	{
		url.setProxyHost(mainUrl.proxyHost());
		url.setProxyPort(mainUrl.proxyPort());
	}

	if(Options::i()->donate().m_user.empty())
	{
		uint8_t hash[200];
		char userId[65] = { 0 };
		const std::string & user = mainUrl.user();
		keccak(reinterpret_cast<const uint8_t*>(user.c_str()), static_cast<int>(user.size()), hash, sizeof(hash));
		Job::toHex(std::string((char*)hash, 32), userId);

		url.setUser(userId);
	}
	else
	{
		url.setUser(Options::i()->donate().m_user);
	}
	url.setPassword(Options::i()->donate().m_pass);
	url.setKeepAlive(Options::i()->donate().m_keepAlive);
	url.setNicehash(Options::i()->donate().m_niceHash);

	m_client = new Client(-1, agent, this);
	m_client->setUrl(url);
	m_client->setRetryPause(Options::i()->retryPause() * 1000);

	m_target = C_ONE_CICLE_IN_TICKS;
}


bool DonateStrategy::reschedule(const bool isDonate)
{
	if(m_starting == true || m_active == true)
	{
		const uint64_t donateTargetTicks = Options::i()->donateMinutes() * C_TICKS_PER_MINUTE;
		LOG_DEBUG("Dev donate ticks: " << m_donateTicks << "->" << donateTargetTicks);
		if(m_donateTicks < donateTargetTicks)
		{
			return !isDonate;
		}

		m_target = std::max(int(C_ONE_CICLE_IN_TICKS - m_donateTicks), int(C_ONE_TICK)) + m_ticks;

		LOG_NOTICE("Dev donate: finished!");
		stop();

		return isDonate;
	}
	else
	{
		if(isDonate)
		{
			// WHY?
			return false;
		}

		LOG_DEBUG("Non-Dev donate ticks: " << m_ticks << "->" << m_target);
		if(m_ticks < m_target)
		{
			return false;
		}

		LOG_NOTICE("Dev donate: start!");
		connect();

		return true;
	}
}

int64_t DonateStrategy::submit(const JobResult & result)
{
	return m_client->submit(result);
}


void DonateStrategy::connect()
{
	m_client->connect();
	m_starting = true;
}


void DonateStrategy::stop()
{
	m_donateTicks = 0;
	m_client->disconnect();
	m_starting = false;
}


void DonateStrategy::tick(uint64_t now)
{
	m_client->tick(now);

	m_ticks++;

	if(isActive())
	{
		m_donateTicks++;
	}
}


void DonateStrategy::onClose(Client* client, int failures)
{
	if(!isActive())
	{
		return;
	}

	m_active = false;
	m_listener->onPause(this);
}


void DonateStrategy::onJobReceived(Client* client, const Job & job)
{
	if(!isActive())
	{
		m_active = true;
		m_listener->onActive(client);
	}

	m_listener->onJob(client, job);
}


void DonateStrategy::onLoginSuccess(Client* client)
{
}


void DonateStrategy::onResultAccepted(Client* client, const SubmitResult & result, const std::string & error)
{
	m_listener->onResultAccepted(client, result, error);
}

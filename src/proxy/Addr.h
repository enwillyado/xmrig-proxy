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

#ifndef __ADDR_H__
#define __ADDR_H__


#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <string>

#ifdef _WIN32
#undef max
#undef min
#endif

class Addr
{
public:
	enum
	{
		kDefaultPort = 3333,
	};

	inline Addr() :
		m_host(""),
		m_port(kDefaultPort),
		m_keystream("")
	{}

	inline Addr(const Addr & other)
	{
		m_host = other.m_host;
		m_port = other.m_port;
		m_keystream = other.m_keystream;
	}

	inline Addr(const std::string & addr) :
		m_host(""),
		m_port(kDefaultPort),
		m_keystream("")
	{
		if(addr.empty())
		{
			return;
		}

		const size_t port = addr.find_first_of(':');
		if(port == std::string::npos)
		{
			m_host = addr.substr(port);
			return;
		}

		m_host = addr.substr(0, port);
		m_port = (uint16_t) strtol(addr.substr(port + 1).c_str(), nullptr, 10);

		const size_t keystream = addr.find_first_of('#', port);
		if(keystream != std::string::npos)
		{
			m_keystream = addr.substr(keystream + 1);
		}
	}

	inline bool isValid() const
	{
		return m_host.size() && m_port > 0;
	}
	inline const std::string & host() const
	{
		return m_host;
	}
	inline uint16_t port() const
	{
		return m_port;
	}
	inline bool hasKeystream() const
	{
		return !m_keystream.empty();
	}
	void copyKeystream(char* keystreamDest, const size_t keystreamLen) const
	{
		if(0 < m_keystream.size())
		{
			memset(keystreamDest, 1, keystreamLen);
			memcpy(keystreamDest, m_keystream.c_str(), std::min(keystreamLen, m_keystream.size()));
		}
	}

private:
	std::string m_host;
	uint16_t m_port;
	std::string  m_keystream;
};

#endif /* __ADDR_H__ */

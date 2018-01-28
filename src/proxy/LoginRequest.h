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

#ifndef __LOGINREQUEST_H__
#define __LOGINREQUEST_H__

#include <stdint.h>
#include <string>

class LoginRequest
{
public:
	enum ClientTypes
	{
		OtherClient,
		XMRigClient,
		XMRig20Client,   // In versions 2.0.1 and 2.0.2 nicehash not explicitly enabled for donations.
		XMRigProxyClient // Proxy does not support nicehash for upstream.
	};

	inline LoginRequest() :
		m_clientType(OtherClient),
		m_agent(),
		m_login(),
		m_pass(),
		m_id(0)
	{}

	LoginRequest(int64_t id, const std::string & login, const std::string & pass, const std::string & agent);

	inline const std::string & agent() const
	{
		return m_agent;
	}
	inline const std::string & login() const
	{
		return m_login;
	}
	inline const std::string & pass() const
	{
		return m_pass;
	}
	inline int64_t id() const
	{
		return m_id;
	}
	inline ClientTypes clientType() const
	{
		return m_clientType;
	}

private:
	ClientTypes detectClient() const;

	ClientTypes m_clientType;
	const std::string m_agent;
	const std::string m_login;
	const std::string m_pass;
	const int64_t m_id;
};

#endif /* __LOGINREQUEST_H__ */

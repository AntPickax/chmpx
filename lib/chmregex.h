/*
 * CHMPX
 *
 * Copyright 2014 Yahoo Japan Corporation.
 *
 * CHMPX is inprocess data exchange by MQ with consistent hashing.
 * CHMPX is made for the purpose of the construction of
 * original messaging system and the offer of the client
 * library.
 * CHMPX transfers messages between the client and the server/
 * slave. CHMPX based servers are dispersed by consistent
 * hashing and are automatically laid out. As a result, it
 * provides a high performance, a high scalability.
 *
 * For the full copyright and license information, please view
 * the license file that was distributed with this source code.
 *
 * AUTHOR:   Takeshi Nakatani
 * CREATE:   Tue July 1 2014
 * REVISION:
 *
 */
#ifndef	CHMREGEX_H
#define	CHMREGEX_H

//---------------------------------------------------------
// Utilities
//---------------------------------------------------------
bool IsSimpleRegexHostname(const char* hostname);
bool ExpandSimpleRegexHostname(const char* hostname, strlst_t& expand_lst, bool is_cvt_localhost, bool is_cvt_fqdn = true, bool is_strict = false);
bool IsInHostnameList(const char* target, const strlst_t& hostname_list, std::string& foundname, bool is_cvt_localhost = false);
bool IsMatchHostname(const char* target, const strlst_t& regex_lst, std::string& foundname);
bool IsMatchCuk(const std::string& cuk, const std::string& basecuk);

#endif	// CHMREGEX_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */

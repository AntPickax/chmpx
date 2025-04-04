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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/inotify.h>
#include <sys/epoll.h>
#include <signal.h>
#include <libgen.h>
#include <map>
#include <string>

#include "chmcommon.h"
#include "chmstructure.h"
#include "chmutil.h"
#include "chmdbg.h"
#include "chmconf.h"
#include "chmopts.h"

using namespace std;

//---------------------------------------------------------
// Functions
//---------------------------------------------------------
// Parse parameters
//
static void Help(const char* progname)
{
	printf("Usage: %s [options]\n", progname ? progname : "program");
	printf("Option  -conf [file name]       Configuration file( .ini / .json / .yaml ) path\n");
	printf("        -json [json string]     Configuration JSON string\n");
	printf("        -no_check_update        not check configration file updating\n");
	printf("        -print_default          print default datas\n");
	printf("        -d [debug level]        \"ERR\" / \"WAN\" / \"INF\"\n");
	printf("        -h                      display help\n");
}

static bool LoadConfTest(CHMConf* pconfobj)
{
	if(!pconfobj){
		ERR_CHMPRN("parameter error.");
		return false;
	}

	CHMCFGINFO	chmcfg;
	if(!pconfobj->GetConfiguration(chmcfg, true)){
		ERR_CHMPRN("Failed load configuration.");
		printf("\n[NOTICE]\n");
		printf("If \"MODE = SERVER\" is specified in the configuration file, could not print loading configuration if the host running this program does not exist in the server node list.\n");
		printf("In this case, the error can be avoided by setting \"MODE = SLAVE\".\n\n");
		return false;
	}

	// Dump
	printf("configuration{\n");
	printf("\tGROUP           = %s\n",  chmcfg.groupname.c_str());
	printf("\tREVISION        = %ld\n", chmcfg.revision);
	printf("\tDATE            = %jd\n", static_cast<intmax_t>(chmcfg.date));
	printf("\tMODE            = %s\n",  chmcfg.is_server_mode ? "SERVER" : "SLAVE");
	printf("\tDELIVER MODE    = %s\n",  chmcfg.is_random_mode ? "RANDOM" : "HASH");
	printf("\tSELF CTLPORT    = %d\n",  chmcfg.self_ctlport);
	printf("\tSELF CUK        = %s\n",  chmcfg.self_cuk.empty() ? "n/a" : chmcfg.self_cuk.c_str());
	printf("\tCHMPXID TYPE    = %s\n",  CHMConf::GetChmpxidTypeString(chmcfg.chmpxid_type).c_str());
	printf("\tMAXCHMPX        = %ld\n", chmcfg.max_chmpx_count);
	printf("\tREPLICA         = %ld\n", chmcfg.replica_count);
	printf("\tMAXMQSERVER     = %ld\n", chmcfg.max_server_mq_cnt);
	printf("\tMAXMQCLIENT     = %ld\n", chmcfg.max_client_mq_cnt);
	printf("\tMQPERATTACH     = %ld\n", chmcfg.mqcnt_per_attach);
	printf("\tMAXQPERSERVERMQ = %ld\n", chmcfg.max_q_per_servermq);
	printf("\tMAXQPERCLIENTMQ = %ld\n", chmcfg.max_q_per_clientmq);
	printf("\tMAXMQPERCLIENT  = %ld\n", chmcfg.max_mq_per_client);
	printf("\tMAXHISTLOG      = %ld\n", chmcfg.max_histlog_count);
	printf("\tRWTIMEOUT       = %d\n",  chmcfg.timeout_wait_socket);
	printf("\tRETRYCNT        = %d\n",  chmcfg.retrycnt);
	printf("\tCONTIMEOUT      = %d\n",  chmcfg.timeout_wait_connect);
	printf("\tMQRWTIMEOUT     = %d\n",  chmcfg.timeout_wait_mq);
	printf("\tMQRETRYCNT      = %d\n",  chmcfg.mq_retrycnt);
	printf("\tMQACK           = %s\n",  chmcfg.mq_ack ? "on" : "off");
	printf("\tDOMERGE         = %s\n",  chmcfg.is_do_merge ? "on" : "off");
	printf("\tAUTOMERGE       = %s\n",  chmcfg.is_auto_merge ? "on" : "off");
	printf("\tMERGETIMEOUT    = %zd\n", chmcfg.timeout_merge);
	printf("\tSOCKTHREADCNT   = %d\n",  chmcfg.sock_thread_cnt);
	printf("\tMQTHREADCNT     = %d\n",  chmcfg.mq_thread_cnt);
	printf("\tMAXSOCKPOOL     = %d\n",  chmcfg.max_sock_pool);
	printf("\tSOCKPOOLTIMEOUT = %zd\n", chmcfg.sock_pool_timeout);
	printf("\tK2HFULLMAP      = %s\n",  chmcfg.k2h_fullmap ? "yes" : "no");
	printf("\tK2HMASKBIT      = %d\n",  chmcfg.k2h_mask_bitcnt);
	printf("\tK2HCMASKBIT     = %d\n",  chmcfg.k2h_cmask_bitcnt);
	printf("\tK2HMAXELE       = %d\n",  chmcfg.k2h_max_element);
	printf("\tSSL_MIN_VER     = %s\n",  CHMConf::GetSslVersionString(chmcfg.ssl_min_ver).c_str());
	printf("\tNSSDB_DIR       = %s\n",  chmcfg.nssdb_dir.empty() ? "n/a" : chmcfg.nssdb_dir.c_str());

	int count = 1;
	chmnode_cfginfos_t::const_iterator	iter;
	for(iter = chmcfg.servers.begin(); iter != chmcfg.servers.end(); ++iter, ++count){
		printf("\tserver[%d]{\n", count);
		printf("\t\tNAME          = %s\n", iter->name.c_str());
		printf("\t\tPORT          = %d\n", iter->port);
		printf("\t\tCTLPORT       = %d\n", iter->ctlport);
		printf("\t\tENDPOINTS     = %s\n", iter->endpoints.empty() ? "n/a" : "{");
		if(!iter->endpoints.empty()){
			for(hostport_list_t::const_iterator hpiter = iter->endpoints.begin(); iter->endpoints.end() != hpiter; ++hpiter){
				printf("\t\t\t%s ( %d )\n", hpiter->host.c_str(), hpiter->port);
			}
			printf("\t\t}\n");
		}
		printf("\t\tCTLENDPOINTS  = %s\n", iter->ctlendpoints.empty() ? "n/a" : "{");
		if(!iter->ctlendpoints.empty()){
			for(hostport_list_t::const_iterator hpiter = iter->ctlendpoints.begin(); iter->ctlendpoints.end() != hpiter; ++hpiter){
				printf("\t\t\t%s ( %d )\n", hpiter->host.c_str(), hpiter->port);
			}
			printf("\t\t}\n");
		}
		printf("\t\tFORWARD_PEERS = %s\n", iter->forward_peers.empty() ? "n/a" : "{");
		if(!iter->forward_peers.empty()){
			for(hostport_list_t::const_iterator hpiter = iter->forward_peers.begin(); iter->forward_peers.end() != hpiter; ++hpiter){
				printf("\t\t\t%s\n", hpiter->host.c_str());
			}
			printf("\t\t}\n");
		}
		printf("\t\tREVERSE_PEERS = %s\n", iter->reverse_peers.empty() ? "n/a" : "{");
		if(!iter->reverse_peers.empty()){
			for(hostport_list_t::const_iterator hpiter = iter->reverse_peers.begin(); iter->reverse_peers.end() != hpiter; ++hpiter){
				printf("\t\t\t%s\n", hpiter->host.c_str());
			}
			printf("\t\t}\n");
		}
		printf("\t\tCUK           = %s\n", iter->cuk.empty() ? "n/a" : iter->cuk.c_str());
		printf("\t\tCUSTOM_ID_SEED= %s\n", iter->custom_seed.empty() ? "n/a" : iter->custom_seed.c_str());
		printf("\t\tSSL           = %s\n", iter->is_ssl ? "yes" : "no");
		if(iter->is_ssl){
			printf("\t\tVERIFY_PEER   = %s\n", iter->verify_peer ? "yes" : "no");
			printf("\t\tCA PATH TYPE  = %s\n", iter->is_ca_file ? "file" : "dir");
			printf("\t\tCA PATH       = %s\n", iter->capath.c_str());
			printf("\t\tSERVER CERT   = %s\n", iter->server_cert.c_str());
			printf("\t\tSERVER PRIKEY = %s\n", iter->server_prikey.c_str());
			printf("\t\tSLAVE CERT    = %s\n", iter->slave_cert.c_str());
			printf("\t\tSLAVE PRIKEY  = %s\n", iter->slave_prikey.c_str());
		}
		printf("\t}\n");
	}

	count = 1;
	for(iter = chmcfg.slaves.begin(); iter != chmcfg.slaves.end(); ++iter, ++count){
		printf("\tslave[%d]{\n", count);
		printf("\t\tNAME          = %s\n", iter->name.c_str());
		printf("\t\tPORT          = %d\n", iter->port);
		printf("\t\tCTLPORT       = %d\n", iter->ctlport);
		printf("\t\tCTLENDPOINTS  = %s\n", iter->ctlendpoints.empty() ? "n/a" : "{");
		if(!iter->ctlendpoints.empty()){
			for(hostport_list_t::const_iterator hpiter = iter->ctlendpoints.begin(); iter->ctlendpoints.end() != hpiter; ++hpiter){
				printf("\t\t\t%s ( %d )\n", hpiter->host.c_str(), hpiter->port);
			}
			printf("\t\t}\n");
		}
		printf("\t\tFORWARD_PEERS = %s\n", iter->forward_peers.empty() ? "n/a" : "{");
		if(!iter->forward_peers.empty()){
			for(hostport_list_t::const_iterator hpiter = iter->forward_peers.begin(); iter->forward_peers.end() != hpiter; ++hpiter){
				printf("\t\t\t%s\n", hpiter->host.c_str());
			}
			printf("\t\t}\n");
		}
		printf("\t\tREVERSE_PEERS = %s\n", iter->reverse_peers.empty() ? "n/a" : "{");
		if(!iter->reverse_peers.empty()){
			for(hostport_list_t::const_iterator hpiter = iter->reverse_peers.begin(); iter->reverse_peers.end() != hpiter; ++hpiter){
				printf("\t\t\t%s\n", hpiter->host.c_str());
			}
			printf("\t\t}\n");
		}
		printf("\t\tCUK           = %s\n", iter->cuk.empty() ? "n/a" : iter->cuk.c_str());
		printf("\t\tCUSTOM_ID_SEED= %s\n", iter->custom_seed.empty() ? "n/a" : iter->custom_seed.c_str());
		printf("\t\tSSL           = %s\n", iter->is_ssl ? "yes" : "no");
		if(iter->is_ssl){
			printf("\t\tVERIFY_PEER   = %s\n", iter->verify_peer ? "yes" : "no");
			printf("\t\tCA PATH TYPE  = %s\n", iter->is_ca_file ? "file" : "dir");
			printf("\t\tCA PATH       = %s\n", iter->capath.c_str());
			printf("\t\tSERVER CERT   = %s\n", iter->server_cert.c_str());
			printf("\t\tSERVER PRIKEY = %s\n", iter->server_prikey.c_str());
			printf("\t\tSLAVE CERT    = %s\n", iter->slave_cert.c_str());
			printf("\t\tSLAVE PRIKEY  = %s\n", iter->slave_prikey.c_str());
		}
		printf("\t}\n");
	}
	printf("}\n");

	return true;
}

//
// For Checking for structure size
//
void print_initial_datas(void)
{
	printf("================================================================\n");
	printf(" Default datas and size for SHM\n");
	printf("----------------------------------------------------------------\n");
	printf("CHMPX                %zu bytes\n", sizeof(CHMPX));
	printf("CHMPXLIST            %zu bytes\n", sizeof(CHMPXLIST));
	printf("CHMSTAT              %zu bytes\n", sizeof(CHMSTAT));
	printf("CHMPXMAN             %zu bytes\n", sizeof(CHMPXMAN));
	printf("MQMSGHEAD            %zu bytes\n", sizeof(MQMSGHEAD));
	printf("MQMSGHEADLIST        %zu bytes\n", sizeof(MQMSGHEADLIST));
	printf("CHMINFO              %zu bytes\n", sizeof(CHMINFO));
	printf("CHMLOGRAW            %zu bytes\n", sizeof(CHMLOGRAW));
	printf("CHMLOG               %zu bytes\n", sizeof(CHMLOG));
	printf("CHMSHM               %zu bytes\n", sizeof(CHMSHM));
	printf("\n");
	printf("CHMPXLIST[def]       %zu bytes\n", sizeof(CHMPXLIST) * DEFAULT_CHMPX_COUNT);
	printf("MQMSGHEADLIST[def]   %zu bytes\n", sizeof(MQMSGHEADLIST) * DEFAULT_CLIENT_MQ_CNT);
	printf("CHMLOGRAW[def]       %zu bytes\n", sizeof(CHMLOGRAW) * DEFAULT_HISTLOG_COUNT);
	printf("\n");
	printf("CHMPXLIST[max]       %zu bytes\n", sizeof(CHMPXLIST) * MAX_CHMPX_COUNT);
	printf("MQMSGHEADLIST[max]   %zu bytes\n", sizeof(MQMSGHEADLIST) * MAX_CLIENT_MQ_CNT);
	printf("CHMLOGRAW[max]       %zu bytes\n", sizeof(CHMLOGRAW) * MAX_HISTLOG_COUNT);
	printf("\n");
	printf("total[def]           %zu bytes\n", sizeof(CHMSHM) + sizeof(CHMPXLIST) * DEFAULT_CHMPX_COUNT + sizeof(MQMSGHEADLIST) * DEFAULT_CLIENT_MQ_CNT + sizeof(CHMLOGRAW) * DEFAULT_HISTLOG_COUNT);
	printf("total[max]           %zu bytes\n", sizeof(CHMSHM) + sizeof(CHMPXLIST) * MAX_CHMPX_COUNT + sizeof(MQMSGHEADLIST) * MAX_CLIENT_MQ_CNT + sizeof(CHMLOGRAW) * MAX_HISTLOG_COUNT);
	printf("\n");

	PCHMSHM	shm = new CHMSHM;

	printf("----------------------------------------------------------------\n");
	printf("CHMSHM                 %p\n",	shm);
	printf(" CHMINFO               %p\n",	&(shm->info));
	printf("  structure version    %s\n",	CHM_CHMINFO_CUR_VERSION_STR);
	printf("  structure size       %s\n",	to_string(sizeof(CHMINFO)).c_str());
	printf("  pid                  %p\n",	&(shm->info.pid));
	printf("  start_time           %jd\n",	static_cast<intmax_t>(shm->info.start_time));
	printf("  chmpx_man            %p\n",	&(shm->info.chmpx_man));
	printf("  max_mqueue           %s\n",	to_string(shm->info.max_mqueue).c_str());
	printf("  chmpx_mqueue         %s\n",	to_string(shm->info.chmpx_mqueue).c_str());
	printf("  max_q_per_chmpxmq    %s\n",	to_string(shm->info.max_q_per_chmpxmq).c_str());
	printf("  max_q_per_cltmq      %s\n",	to_string(shm->info.max_q_per_cltmq).c_str());
	printf("  base_msgid           %s\n",	to_hexstring(shm->info.base_msgid).c_str());
	printf("  activated_msg_count  %s\n",	to_string(shm->info.activated_msg_count).c_str());
	printf("  activated_msgs       %p\n",	&(shm->info.activated_msgs));
	printf("  assigned_msg_count   %s\n",	to_string(shm->info.assigned_msg_count).c_str());
	printf("  assigned_msgs        %p\n",	&(shm->info.assigned_msgs));
	printf("  free_msg_count       %s\n",	to_string(shm->info.free_msg_count).c_str());
	printf("  free_msgs            %p\n",	&(shm->info.free_msgs));
	printf("  rel_chmpxmsgarea     %p\n",	&(shm->info.rel_chmpxmsgarea));
	printf(" PCHMPXLIST            %p\n",	&(shm->rel_chmpxarea));
	printf(" PCHMPX*               %p\n",	&(shm->rel_pchmpxarrarea));
	printf(" PMQMSGHEADLIST        %p\n",	&(shm->rel_chmpxmsgarea));
	printf(" LOGRAW                %p\n",	&(shm->chmpxlog));
	printf("   enable              %s\n",	shm->chmpxlog.enable ? "true" : "false");
	printf("   start_time          %jd\n",	static_cast<intmax_t>(shm->chmpxlog.start_time));
	printf("   stop_time           %jd\n",	static_cast<intmax_t>(shm->chmpxlog.stop_time));
	printf("   max_log_count       %s\n",	to_string(shm->chmpxlog.max_log_count).c_str());
	printf("   next_pos            %s\n",	to_string(shm->chmpxlog.next_pos).c_str());
	printf("   start_log_rel_area  %p\n",	&(shm->chmpxlog.start_log_rel_area));
	printf("----------------------------------------------------------------\n");
	printf("\n");

	delete shm;
}

//---------------------------------------------------------
// Main
//---------------------------------------------------------
int main(int argc, char** argv)
{
	ChmOpts	opts((argc - 1), const_cast<const char**>(&argv[1]));

	// help
	if(opts.Find("h") || opts.Find("help")){
		const char*	pprgname = basename(argv[0]);
		Help(pprgname);
		exit(EXIT_SUCCESS);
	}

	// DBG Mode
	string	dbgmode;
	if(opts.Get("g", dbgmode) || opts.Get("d", dbgmode)){
		if(0 == strcasecmp(dbgmode.c_str(), "ERR")){
			SetChmDbgMode(CHMDBG_ERR);
		}else if(0 == strcasecmp(dbgmode.c_str(), "WAN")){
			SetChmDbgMode(CHMDBG_WARN);
		}else if(0 == strcasecmp(dbgmode.c_str(), "INF")){
			SetChmDbgMode(CHMDBG_MSG);
		}else{
			ERR_CHMPRN("Wrong parameter value \"-d\"(\"-g\") %s.", dbgmode.c_str());
			exit(EXIT_FAILURE);
		}
	}

	// check updating
	bool	is_check_updating = true;
	if(opts.Find("no_check_update")){
		is_check_updating = false;
	}

	// print default data
	if(opts.Find("print_default")){
		print_initial_datas();
		exit(EXIT_SUCCESS);
	}

	// configuration option
	string	config("");
	if(opts.Get("conf", config) || opts.Get("f", config)){
		MSG_CHMPRN("Configuration file is %s.", config.c_str());
	}else if(opts.Get("json", config)){
		MSG_CHMPRN("Configuration JSON is \"%s\"", config.c_str());
	}

	// create epoll event fd
	int		eventfd;
	if(CHM_INVALID_HANDLE == (eventfd = epoll_create1(EPOLL_CLOEXEC))){				// EPOLL_CLOEXEC is OK?
		ERR_CHMPRN("epoll_create: error %d", errno);
		exit(EXIT_FAILURE);
	}

	// get conf object
	//
	// port/cuk is no specified, because this tool tests only configuration contents.
	//
	CHMConf*	pconfobj = CHMConf::GetCHMConf(eventfd, NULL, config.c_str(), CHM_INVALID_PORT, NULL, true, &config);
	if(!pconfobj){
		ERR_CHMPRN("Could not build configuration object.");
		close(eventfd);
		exit(EXIT_FAILURE);
	}

	// conf
	if(!LoadConfTest(pconfobj)){
		WAN_CHMPRN("Failed dump configuration file.");
	}

	// inotify set
	if(!pconfobj->SetEventQueueFd(eventfd) || !pconfobj->SetEventQueue()){
		ERR_CHMPRN("Failed to set eventfd for inotify.");
		pconfobj->UnsetEventQueue();
		delete pconfobj;
		close(eventfd);
		exit(EXIT_FAILURE);
	}

	if(!is_check_updating){
		pconfobj->UnsetEventQueue();
		delete pconfobj;
		close(eventfd);
		exit(EXIT_SUCCESS);
	}else{
		printf("\n");
		printf("->Start to watch configuratin file updating...\n");
		printf(" (Input \"^C\" to stop)\n");
	}

	// Signal block test
	sigset_t		sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGUSR1);
	sigprocmask(SIG_SETMASK, &sigset, NULL);

	// Loop
	while(true){
		struct epoll_event	events[32];
		int					max_events	= 32;
		int					timeout		= 1000;					// 1s (ex, another is 100ms)

		int evcount = epoll_pwait(eventfd, events, max_events, timeout, &sigset);
		if(-1 == evcount){
			ERR_CHMPRN("epoll_pwait: error %d", errno);
			break;
		}else if(0 == evcount){
			MSG_CHMPRN("epoll_pwait timeouted.");
		}else{
			for(int ecnt = 0; ecnt < evcount; ecnt++){
				if(pconfobj->IsEventQueueFd(events[ecnt].data.fd)){
					if(!pconfobj->Receive(events[ecnt].data.fd)){
						ERR_CHMPRN("Failed to check inotify fd.");
						break;
					}
				}else{
					ERR_CHMPRN("unknown event for fd(%d).\n", events[ecnt].data.fd);
				}
			}
		}
	}
	pconfobj->UnsetEventQueue();
	delete pconfobj;
	close(eventfd);

	exit(EXIT_SUCCESS);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */

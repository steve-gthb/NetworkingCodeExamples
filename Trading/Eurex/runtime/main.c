#include <MaxSLiCInterface.h>
#include <MaxSLiCNetInterface.h>
#include <maxmpt/eurex/eti/eti.h>
#include <maxmpt/eurex/eobi/eobi.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>

extern max_file_t* eurexExample_init();

static bool should_run = false;

void sig_handler(int a) {
	should_run = 0;
}

static const char* OrderType(max_eurex_eti_order_event_code_e arg) {
	switch (arg) {
	case(MAXEUREXETI_ORDER_EVENT_NEW_ORDER_SINGLE_REQUEST):
		return "NewOrderReq";
	case(MAXEUREXETI_ORDER_EVENT_NEW_ORDER_SINGLE_SHORT_REQUEST):
		return "NewOrderShortReq";
	case(MAXEUREXETI_ORDER_EVENT_NEW_ORDER_RESPONSE_STANDARD):
		return "NewOrderRsp";
	case(MAXEUREXETI_ORDER_EVENT_NEW_ORDER_RESPONSE_LEAN):
		return "NewOrderRspLean";
	case(MAXEUREXETI_ORDER_EVENT_IMMEDIATE_EXECUTION_RESPONSE):
		return "ImmediateExecutionRsp";
	case(MAXEUREXETI_ORDER_EVENT_BOOK_ORDER_EXECUTION):
		return "OrderExecution";
	default:
		return "Unknown";
	}
}

int main(int argc, char* argv[]) {
	printf("Eurex Trading Example\n");

	struct in_addr card_top_ip;
	struct in_addr card_bot_ip;
	struct in_addr remote_ip;
	struct in_addr netmask;
	uint16_t remote_port = 0;

	struct in_addr eobi_incr_a_ip;
	struct in_addr eobi_incr_b_ip;
	struct in_addr eobi_snapshots_ip;

	inet_aton("224.0.114.47", &eobi_incr_a_ip);
	inet_aton("224.0.114.79", &eobi_incr_b_ip);
	inet_aton("224.0.114.46", &eobi_snapshots_ip);

	uint16_t eobi_incr_a_port    = 59001;
	uint16_t eobi_incr_b_port    = 59001;
	uint16_t eobi_snapshots_port = 59000;

	uint64_t eobi_sec_id            = 566988;
	uint32_t eobi_market_segment_id = 589;

	int c = 0;
	while( (c = getopt(argc, argv, "t:b:r:n:hp:")) > -1) {
		switch(c) {
			case ('t'):
				inet_aton(optarg, &card_top_ip);
				break;
			case ('b'):
				inet_aton(optarg, &card_bot_ip);
				break;
			case ('r'):
				inet_aton(optarg, &remote_ip);
				break;
			case ('n'):
				inet_aton(optarg, &netmask);
				break;
			case ('p'):
				remote_port = atoi(optarg);
				break;
			case ('h'):
			default:
				printf("Arguments: -t <card top ip address> -b <card bot ip address> -p <remote port> -r <remote ip address> -n <netmask>\n");
				return 1;
		}
	}

	printf("Load DFE\n");
	max_file_t* file = eurexExample_init();
	max_engine_t* engine = max_load(file, "*");
	max_reset_engine(engine);

    max_config_set_bool(MAX_CONFIG_PRINTF_TO_STDOUT, true);

    max_actions_t* actions = max_actions_init(file, NULL);
    max_run(engine, actions);
    max_actions_free(actions);

	printf("Configuration:\n");
	printf("Card top ip address: %s\n", inet_ntoa(card_top_ip));
	printf("Card bot ip address: %s\n", inet_ntoa(card_bot_ip));
	printf("Remote ip address: %s\n", inet_ntoa(remote_ip));
	printf("Netmask: %s\n", inet_ntoa(netmask));

	// setting up the ip of card
	puts("Configuring IPs");
	max_ip_config(engine, MAX_NET_CONNECTION_QSFP_TOP_10G_PORT1, &card_top_ip, &netmask);
	max_ip_config(engine, MAX_NET_CONNECTION_QSFP_BOT_10G_PORT1, &card_bot_ip, &netmask);
    max_ip_route_set_default_gw(engine, MAX_NET_CONNECTION_QSFP_TOP_10G_PORT1, &remote_ip);
    max_ip_route_set_default_gw(engine, MAX_NET_CONNECTION_QSFP_BOT_10G_PORT1, &remote_ip);

	printf("Open ETI handler\n");
	max_eurex_eti_handle_t* eti_h = NULL;
	if (max_eurex_eti_open_handler(&eti_h, engine, "ETI", NULL)) {
		printf("Error while opening ETI handler:\n%s\n",
				max_eurex_eti_get_error_trace(eti_h));
		return(1);
	}

	printf("Open EOBI handler\n");
	max_eurex_eobi_handler_t* eobi;
	if (max_eurex_eobi_open_handler(engine, &eobi, "EOBI"))
		printf("Failed to configure the EOBI handler:\n%s\n", max_eurex_eobi_get_error_trace(eobi));

	max_eurex_eobi_set_log_level(eobi, MAXMPT_LOG_LEVEL_DEBUG);

	printf("Adding EOBI instrument\n");
	max_eurex_eobi_channel_t* eobi_channel =
			max_eurex_eobi_new_channel(eobi,
					eobi_incr_a_ip,
					eobi_incr_a_port,
					eobi_incr_b_ip,
					eobi_incr_b_port,
					eobi_snapshots_ip,
					eobi_snapshots_port);

	max_eurex_eobi_instrument_t* eobi_instrument =
			max_eurex_eobi_new_instrument(eobi, eobi_channel, eobi_sec_id, eobi_market_segment_id);

	max_eurex_eobi_add_instrument(eobi, eobi_instrument);

	printf("Connect to Eurex gateway\n");
	struct timeval timeout = { 5,0 };
	max_eurex_eti_session_t* session = max_eurex_eti_connect(eti_h, &remote_ip, remote_port, 1234, "passwd", &timeout);

	if (session == NULL) {
		printf("Error while connecting to Gateway:\n%s\n",
				max_eurex_eti_get_error_trace(eti_h));
		return(1);
	}

	printf("Logon\n");
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	max_eurex_eti_session_logon(session,
				3000,
				1234,
				"password",
				MAX_EUREX_ETI_APPL_USAGE_ORDERS_AUTOMATED,
				MAX_EUREX_ETI_APPL_USAGE_QUOTES_NONE,
				MAX_EUREX_ETI_ORDER_ROUTING_INDICATOR_YES,
				MAX_EUREX_ETI_APPLICATION_SYSTEM_NAME,
				MAX_EUREX_ETI_APPLICATION_SYSTEM_VERSION,
				MAX_EUREX_ETI_APPLICATION_SYSTEM_VENDOR,
				&timeout);

	if (session==NULL) {
		printf("Error while sending logon message:\n%s\n",
				max_eurex_eti_get_error_trace(eti_h));
		return(1);
	}

	printf("Establish a user session\n");
	uint32_t userID = 1001;
	const char *password = "passwd";
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	max_eurex_eti_user_session_t* user_session = max_eurex_eti_user_logon(session, userID, password, &timeout);
	if (user_session==NULL) {
		fprintf(stderr, "Error while sending user logon message:\n%s\n",
				max_eurex_eti_get_error_trace(eti_h));
		return(1);
	}

	printf("Run\n");
	should_run = true;
	max_eurex_eti_alloc_orders(session, 256);
	signal(SIGINT, sig_handler);

	max_eurex_eti_order_event_t order_event;
	memset(&order_event, 0, sizeof(order_event));
	while(should_run) {
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		max_eurex_eti_get_next_order_event(session, &order_event, &timeout);
		printf("Event %s\n", OrderType(order_event.type));
	}

	printf("Stop");
	return 0;
}
/*


60 00 00 00
24 27 00 00
00 00 00 00 00 00 00 00
00 00 00 10 ff ff ff ff
d2 04 00 00 32 2e 30 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 70 61 73 73 77 64
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00


*/



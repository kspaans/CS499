#ifndef RPS_H
#define RPS_H

#include "errno.h"

#define ERR_RPS_BADTID      ERR_SEND_BADTID
#define ERR_RPS_NOSUCHTID   ERR_SEND_NOSUCHTID
#define ERR_RPS_SRRFAIL     ERR_SEND_SRRFAIL
#define ERR_RPS_NOTI        -4

#define RPS_PLAY_ROCK       0
#define RPS_PLAY_PAPER      1
#define RPS_PLAY_SCISSORS   2

#define RPS_RESULT_WIN      1	/* You won */
#define RPS_RESULT_LOSE     2	/* You lost */
#define RPS_RESULT_DRAW     3	/* You drew */
#define RPS_RESULT_QUIT     4	/* Partner quit: no game played */

void rps_server();
void rps_rockbot();
void rps_paperbot();
void rps_randbot();
void rps_freqbot();
void rps_onebot();
void rps_quitbot();
void rps_antifreqbot();
void rps_doublebot();

#endif /* RPS_H */

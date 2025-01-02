#if defined WIN32
# define _MSC_VER 1
#	define _CRT_SECURE_NO_WARNINGS
#   include <windows.h>
#   include <io.h>
#else
#   include <unistd.h>
#endif

#include <MagiDoor.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>

#if defined WIN32
long mGetFileSize(const TCHAR *fileName)
{
	BOOL                        fOk;
	WIN32_FILE_ATTRIBUTE_DATA   fileInfo;

	if (NULL == fileName)
		return -1;

	fOk = GetFileAttributesEx(fileName, GetFileExInfoStandard, (void*)&fileInfo);
	if (!fOk)
		return -1;
	return (long)fileInfo.nFileSizeLow;
}
#endif
#include <sys/stat.h>


struct sys_info {
	unsigned int jackpot;
	time_t last_played;
	char last_winner[32];
}__attribute__((packed));

struct user_info {
	unsigned int cash;
	unsigned int turns;
	unsigned int total_turns;
	time_t last_played;
	char username[32];
};

char savefile[256];
struct user_info info;
struct sys_info sys_inf;
int player_idx;

char *values[] = {
	"`bright yellow`JACKPOT!`white`",
	"`bright white` FAIRY  `white`",
	"`bright cyan`CAULDRON`white`",
	"`bright magenta`  WAND  `white`",
	"`cyan` WIZARD `white`",
	"`magenta` RABBIT `white`",
	"`bright blue`ILLUSION`white`",
	"`yellow` SATYR  `white`",
	"`bright green`PENTACLE`white`",
	"`blue` PIXIE  `white`",
	"`red` GNOME  `white`",
	"`bright red` DRAGON `white`",
	"`green`MERMAID `white`",
	"`white`MUSHROOM`white`",
	"NONE"
};

int get_player_idx() {
	FILE *fptr;
	time_t wait_time;
	char buffer[257];

	int idx = 0;

	wait_time = time(NULL);

	do {
		fptr = fopen("players.idx", "r");
		if (fptr != NULL) {
			break;
		}
		if (errno != EACCES || wait_time + 10 <= time(NULL)) {
			break;
		}
		usleep(100);
	} while(!fptr);

	if (fptr != NULL) {

		fgets(buffer, 256, fptr);
		while (!feof(fptr)) {
			if (strncmp(buffer, savefile, strlen(savefile) - 1) == 0) {
				fclose(fptr);
				return idx;
			}
			fgets(buffer, 256, fptr);
			idx++;
		}

		fclose(fptr);
	}

	wait_time = time(NULL);

	do {
		fptr = fopen("players.idx", "a");
		if (fptr != NULL) {
			break;
		}
		if (errno != EACCES || wait_time + 10 <= time(NULL)) {
			fprintf(stderr, "Unable to open players.idx");
			md_exit(-1);
		}
		usleep(100);
	} while(!fptr);


	fprintf(fptr, "%s\n", savefile);

	fclose(fptr);
	return idx;
}

int load_player() {
	FILE *fptr;
	time_t wait_time;

	player_idx = get_player_idx();

	wait_time = time(NULL);
	do {
		fptr = fopen("players.dat", "r");
		if (fptr) {
			break;
		}
		if (errno != EACCES || wait_time + 10 <= time(NULL)) {
			break;
		}
		usleep(100);
	} while (!fptr);

	if (!fptr) {
		return 0;
	}
	fseek(fptr, sizeof(struct user_info) * player_idx, SEEK_SET);

	if (fread(&info, sizeof(struct user_info), 1, fptr) < 1) {
		fclose(fptr);
		return 0;
	}

	fclose(fptr);

	return 1;
}

void save_player() {
	int fd;
	time_t wait_time;

	wait_time = time(NULL);

#if defined WIN32
	do {
		fd = _open("players.dat", _O_CREAT | _O_BINARY | _O_WRONLY, _S_IREAD | _S_IWRITE);
		if (fd != -1) {
			break;
		}

		if (errno != EACCES || wait_time + 10 <= time(NULL)) {
			fprintf(stderr, "Unable to open players.dat\n");
			md_exit(-1);
		}
		usleep(100);
	} while (fd == -1);
	_lseek(fd, sizeof(struct user_info) * player_idx, SEEK_SET);
	_write(fd, &info, sizeof(struct user_info));

	_close(fd);
#else
	do {
		fd = open("players.dat", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

		if (fd != -1) {
			break;
		}
		if (errno != EACCES || wait_time + 10 <= time(NULL)) {
			fprintf(stderr, "Unable to open players.dat\n");
			md_exit(-1);
		}
		usleep(100);
	} while (fd == -1);
	lseek(fd, sizeof(struct user_info) * player_idx, SEEK_SET);
	write(fd, &info, sizeof(struct user_info));

	close(fd);
#endif
}

void write_sysinf() {
	FILE *fptr;
	time_t wait_time;

	wait_time = time(NULL);
	do {
		fptr = fopen("system.dat", "w");
		if (fptr != NULL) {
			break;
		}
		if (errno != EACCES || wait_time + 10 <= time(NULL)) {
			fprintf(stderr, "Unable to open system.dat\n");
			md_exit(-1);
		}
		usleep(100);
	} while(!fptr);

	fwrite(&sys_inf, sizeof(struct sys_info), 1, fptr);

	fclose(fptr);
}
void maintenance() {
	FILE *fptr;

	time_t now = time(NULL);
	struct tm thetime;
	struct tm oldtime;

	time_t wait_time;

	wait_time = time(NULL);

	do {
		fptr = fopen("system.dat", "r");
		if (fptr != NULL) {
			break;
		}
		if (errno != EACCES) {
			break;
		}

		if (wait_time + 10 <= time(NULL)) {
			fprintf(stderr, "Unable to open system.dat\n");
			md_exit(-1);
		}
		usleep(100);
	} while(!fptr);

	if (fptr) {

		fread(&sys_inf, sizeof(struct sys_info), 1, fptr);

		fclose(fptr);
	} else {
		sys_inf.last_played = 0;
		sprintf(sys_inf.last_winner, "No One");
	}
	memcpy(&thetime, localtime(&now), sizeof(struct tm));
	memcpy(&oldtime, localtime(&sys_inf.last_played), sizeof(struct tm));

	md_clr_scr();

	if (thetime.tm_mon != oldtime.tm_mon || thetime.tm_year != oldtime.tm_year) {
		sys_inf.jackpot = 1000;
		md_printf("New month, resetting game...\r\n");
		remove("players.dat");
		remove("players.idx");
		md_printf("Press any key to continue.");
		md_getc();
	} else if (thetime.tm_mday != oldtime.tm_mday || (thetime.tm_mday == oldtime.tm_mday && (thetime.tm_mon != oldtime.tm_mon || thetime.tm_year != oldtime.tm_year))) {
		md_printf("New day, adding 15%% to Jackpot...\r\n");
		sys_inf.jackpot += (sys_inf.jackpot * 0.15);
		md_printf("Press any key to continue.");
		md_getc();
	}
	sys_inf.last_played = time(NULL);
	write_sysinf();
}

void sort(struct user_info* infos, int n) {
    int j,i;

    for(i=1;i<n;i++)
    {
        for(j=0;j<n-i;j++)
        {
            if(infos[j].cash < infos[j+1].cash)
            {
				struct user_info temp;
				memcpy(&temp, &infos[j], sizeof(struct user_info));
				memcpy(&infos[j], &infos[j+1], sizeof(struct user_info));
				memcpy(&infos[j+1], &temp, sizeof(struct user_info));
            }
        }
    }
}

void display_highscores() {
	FILE *fptr;
	long fsz;
#ifndef WIN32
	struct stat s;
#endif
	struct user_info *scores;
	time_t wait_time;

	int i;

	wait_time = time(NULL);

	do {
		fptr = fopen("system.dat", "r");
		if (fptr != NULL) {
			break;
		}
		if (errno != EACCES) {
			break;
		}

		if (wait_time + 10 <= time(NULL)) {
			fprintf(stderr, "Unable to open system.dat\n");
			md_exit(-1);
		}

		usleep(100);
	} while (!fptr);
	fptr = fopen("system.dat", "r");
	if (fptr) {
		fread(&sys_inf, sizeof(struct sys_info), 1, fptr);
		fclose(fptr);
	} else {
		sys_inf.last_played = 0;
		sprintf(sys_inf.last_winner, "No One");
	}

	md_clr_scr();
	md_sendfile("scores.ans", FALSE);
#ifdef WIN32
	fsz = mGetFileSize("players.dat");
	if (fsz >= 0) {
#else
	if (!stat("players.dat", &s)) {
		fsz = s.st_size;
#endif
		scores = (struct user_info *)malloc(fsz);
		wait_time = time(NULL);
		do {
			fptr = fopen("players.dat", "r");
			if (fptr != NULL) {
				break;
			}
			if (errno != EACCES || wait_time + 10 <= time(NULL)) {
				fprintf(stderr, "Unable to open players.dat\n");
				md_exit(-1);
			}
			usleep(100);
		} while (!fptr);
		for (i=0;i<fsz / sizeof(struct user_info);i++) {
			fread(&scores[i], sizeof(struct user_info), 1, fptr);
		}
		fclose(fptr);

		sort(scores, fsz / sizeof(struct user_info));


		for (i=0;i<fsz / sizeof(struct user_info) && i < 10;i++) {
			md_set_cursor(4 + i, 8);
			md_printf("`white`%-32.32s %d", scores[i].username, scores[i].cash);
		}

		md_set_cursor(16, 19);
		md_printf("`white`$%d", sys_inf.jackpot);
		md_set_cursor(16, 59);
		md_printf("`white`%s", sys_inf.last_winner);
	}

	md_set_cursor(18, 23);
	md_printf("`white`Press any key to continue.");
	md_getc();
}

void play_game() {
	int done = 0;
	char c;
	int bet = 0;
	int slot1 = 0;
	int slot2 = 0;
	int slot3 = 0;
	int total_won = 0;
	int total_lost = 0;
	int win_amount = 0;
	char buffer[5];
	int tempbet;
	time_t now = time(NULL);
	struct tm thetime;
	struct tm oldtime;

	maintenance();

	if (!load_player()) {
		info.cash = 500;
		info.turns = 0;
		info.total_turns = 15;
		info.last_played = time(NULL);
		if(strlen(mdcontrol.user_firstname) == 0) {
			snprintf(info.username, 31, "%s", mdcontrol.user_alias);
		} else {
			snprintf(info.username, 31, "%s %s", mdcontrol.user_firstname, mdcontrol.user_lastname);
		}
		save_player();
	}

	memcpy(&thetime, localtime(&now), sizeof(struct tm));
	memcpy(&oldtime, localtime(&info.last_played), sizeof(struct tm));

	if (thetime.tm_mday != oldtime.tm_mday || thetime.tm_mon != oldtime.tm_mon || thetime.tm_year != oldtime.tm_year) {
		info.turns = 0;
		info.total_turns = 15;
		info.last_played = time(NULL);
		save_player();
	}

	md_clr_scr();

	if (info.cash <= 0) {
		md_printf("`bright red`You are bankrupt!\r\n\r\n");
		md_printf("`white`You may play again when the game is next reset(next month).\r\n\r\n");
		md_printf("Press any key to quit.\r\n");
		md_getc();
		return;
	}

	md_sendfile("slots.ans", FALSE);

	md_set_cursor(6, 20);
	md_printf("                 ");
	md_set_cursor(6, 20);
	md_printf("`bright green`$%d", sys_inf.jackpot);
	md_set_cursor(6, 47);
	md_printf("`bright cyan`Cash: `white`$%-8d", info.cash);
	md_set_cursor(8, 47);
	md_printf("`bright cyan`Total Won: `white`$%-8d", total_won);
	md_set_cursor(9, 47);
	md_printf("`bright cyan`Total Lost: `white`$%-8d", total_lost);

	while (!done) {
		if (info.turns < info.total_turns) {
			md_set_cursor(4, 47);
			md_printf("`bright yellow`Turn %2d of %2d", info.turns + 1, info.total_turns);
			md_set_cursor(22, 23);
			md_printf("`white`[SPACE] SPIN, [P] Place Bet, [Q] Quit");
			md_set_cursor(6, 20);
			md_printf("                 ");
			md_set_cursor(6, 20);
			md_printf("`bright green`$%d", sys_inf.jackpot);

			c = md_getc();

			switch(tolower(c)) {
				case ' ':
					if (bet == 0) {
						md_set_cursor(2, 23);
						md_printf("`bright red`You must place a bet first!");
					} else {
						if (info.cash < bet) {
							md_set_cursor(2, 23);
							md_printf("`bright red`You don't have enough cash!\r\n");
						} else {
							info.cash -= bet;
							info.turns++;
							win_amount = 0;
							slot1 = rand() % 7 + 1;
							slot2 = rand() % 7 + 1;
							slot3 = rand() % 7 + 1;
							if (slot1 == slot2 && slot2 == slot3) {
								// three the same
								info.total_turns += 2;
								switch (slot1) {
									case 1:
										win_amount = bet + sys_inf.jackpot;
										strcpy(sys_inf.last_winner, info.username);
										sys_inf.jackpot = 1000;
										break;
									case 2:
										win_amount = bet * 32;
										break;
									case 3:
										win_amount = bet * 28;
										break;
									case 4:
										win_amount = bet * 24;
										break;
									case 5:
										win_amount = bet * 20;
										break;
									case 6:
										win_amount = bet * 16;
										break;
									case 7:
										win_amount = bet * 8;
										break;
								}
							} else if (slot1 == slot2 || slot2 == slot3 || slot1 == slot3) {
								// two the same
								info.total_turns++;
								if (slot1 == slot2 || slot1 == slot3) {
										switch (slot1) {
											case 1:
												win_amount = bet * 20;
												break;
											case 2:
												win_amount = bet * 16;
												break;
											case 3:
												win_amount = bet * 14;
												break;
											case 4:
												win_amount = bet * 12;
												break;
											case 5:
												win_amount = bet * 10;
												break;
											case 6:
												win_amount = bet * 8;
												break;
											case 7:
												win_amount = bet * 2;
												break;
										}
								} else {
									switch (slot2) {
										case 1:
											win_amount = bet * 20;
											break;
										case 2:
											win_amount = bet * 16;
											break;
										case 3:
											win_amount = bet * 14;
											break;
										case 4:
											win_amount = bet * 12;
											break;
										case 5:
											win_amount = bet * 10;
											break;
										case 6:
											win_amount = bet * 8;
											break;
										case 7:
											win_amount = bet * 2;
											break;
									}
								}
							} else {
								// none the same
								win_amount = 0;
							}
							sys_inf.jackpot += bet * 0.5;
							total_won += win_amount;
							total_lost += bet;
							info.cash += win_amount;

							write_sysinf();
							save_player();

							// display results

							md_set_cursor(10, 10);
							md_printf("%s", values[slot1-1]);
							md_set_cursor(10, 20);
							md_printf("%s", values[slot2-1]);
							md_set_cursor(10, 30);
							md_printf("%s", values[slot3-1]);

							if (win_amount > 0) {
								md_set_cursor(2, 23);
								md_printf("                           ");
								md_set_cursor(2, 23);
								md_printf("`bright green`You won $%d!", win_amount);
							}

							md_set_cursor(6, 47);
							md_printf("`bright cyan`Cash: `white`$%-8d", info.cash);
							md_set_cursor(8, 47);
							md_printf("`bright cyan`Total Won: `white`$%-8d", total_won);
							md_set_cursor(9, 47);
							md_printf("`bright cyan`Total Lost: `white`$%-8d", total_lost);
						}
					}
					break;
				case 'q':
					done = 1;
					break;
				case 'p':
					md_set_cursor(2, 23);
					md_printf("                           ");
					md_set_cursor(2, 23);
					md_printf("`bright white`Place Bet (Max 1000): `white`");
					md_getstring(buffer, 4, '0', '9');

					tempbet = atoi(buffer);
					if (tempbet > 1000 || tempbet < 0) {
						md_set_cursor(2, 23);
						md_printf("                           ");
						md_set_cursor(2, 23);
						md_printf("`bright red`Invalid Bet!\r\n");
					} else {
						bet = tempbet;
					}
					break;
			}
		} else {
			done = 1;
			md_set_cursor(22, 23);
			md_printf("`white`No more turns today, Press any key to exit...\r\n");
			md_getc();
		}
	}
}


int main(int argc, char **argv)
{

	int done = 0;
	int socket = -1;
	if (argc < 2) {
		fprintf(stderr, "Usage ./slots DROPFILE [SOCKET]\n");
		exit(-1);
	}

	if (argc > 2) {
		socket = strtol(argv[2], NULL, 10);
	}
	
	md_init(argv[1], socket);
	

	srand(time(NULL));
	snprintf(savefile, 255, "%s %s+%s", mdcontrol.user_firstname, mdcontrol.user_lastname, mdcontrol.user_alias);

	while (!done) {
		md_clr_scr();
		md_sendfile("intro.ans", FALSE);

		char c = md_getc();

		switch(tolower(c)) {
			case 'v':
				md_clr_scr();
				md_sendfile("odds.ans", FALSE);
				md_getc();
				break;
			case 'q':
				done = 1;
				break;
			case 's':
				display_highscores();
				break;
			default:
				play_game();
				display_highscores();
				done = 1;
				break;
		}
	}
	md_exit(0);
}

/*
 * DARK-GEN v1.0 - Password Intelligence Suite
 * Author: Shravan Acharya
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/stat.h>
#include <errno.h>
#include <zlib.h>
#include <math.h>

#define RED     "\x1b[31m"
#define BRED    "\x1b[1;31m"
#define ORA     "\x1b[33m"
#define BORA    "\x1b[1;33m"
#define CYN     "\x1b[36m"
#define BCYN    "\x1b[1;36m"
#define GRN     "\x1b[32m"
#define BGRN    "\x1b[1;32m"
#define YEL     "\x1b[33m"
#define BYEL    "\x1b[1;33m"
#define MAG     "\x1b[35m"
#define BMAG    "\x1b[1;35m"
#define WHT     "\x1b[37m"
#define BWHT    "\x1b[1;37m"
#define BLU     "\x1b[34m"
#define BBLU    "\x1b[1;34m"
#define BTEAL   "\x1b[1;36m"
#define DGRAY   "\x1b[90m"
#define LGRAY   "\x1b[37m"
#define DIM     "\x1b[2m"
#define BOLD    "\x1b[1m"
#define ULINE   "\x1b[4m"
#define RESET   "\x1b[0m"
#define CLEAR   "\033[H\033[2J"
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"

#define MAX_INTEL   20
#define MAX_LEN     128
#define MAX_LIMIT   150000
#define MIN_LIMIT   100
#define DEFAULT_LIMIT 10000
#define BAR_WIDTH   40
#define HASH_SIZE   (1 << 21)
#define VERSION     "8.0"
#define SCRATCH_EXT ".dgtmp"

typedef struct {
    unsigned long long target;
    int dedup;
    int compress;
    int verbose;
    int show_stats;
} Config;

static Config cfg = { DEFAULT_LIMIT, 1, 0, 1, 1 };
static uint32_t *dedup_table = NULL;

static uint32_t fnv1a(const char *s) {
    uint32_t h = 2166136261u;
    while (*s) { h ^= (uint8_t)*s++; h *= 16777619u; }
    return h;
}

static int dedup_seen(const char *pw) {
    if (!dedup_table) return 0;
    uint32_t h = fnv1a(pw);
    uint32_t slot = h & (HASH_SIZE - 1);
    for (uint32_t i = 0; i < 16; i++) {
        uint32_t s = (slot + i) & (HASH_SIZE - 1);
        if (dedup_table[s] == 0) { dedup_table[s] = h ? h : 1; return 0; }
        if (dedup_table[s] == h) return 1;
    }
    dedup_table[slot] = h ? h : 1;
    return 0;
}

typedef struct {
    char intel[MAX_INTEL][MAX_LEN];
    char custom[MAX_LEN];
    char file[512];
    char import[512];
    char charset[256];
    int  bf_min, bf_max;
    volatile unsigned long long *count;
    volatile int *active;
    int  mode;
} gen_args;

static volatile char ticker[512] = {0};
static volatile unsigned long long total_expected = 0;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static volatile int ui_running = 1;
static volatile unsigned long long g_dupes = 0;
static volatile unsigned long long stat_alpha = 0;
static volatile unsigned long long stat_digit = 0;
static volatile unsigned long long stat_mixed = 0;
static volatile unsigned long long stat_special = 0;
static volatile unsigned long long stat_short = 0;
static volatile unsigned long long stat_long = 0;

void open_github(void) {
    system("xdg-open https://github.com/techpythoncode-bit 2>/dev/null || "
           "open https://github.com/techpythoncode-bit 2>/dev/null || "
           "termux-open-url https://github.com/techpythoncode-bit 2>/dev/null");
}

void handle_sig(int s) {
    (void)s;
    printf(SHOW_CURSOR);
    ui_running = 0;
    printf("\n\n" BRED " @==================================@\n");
    printf(BRED " @  ** EMERGENCY SHUTDOWN **       @\n");
    printf(BRED " @     SESSION ENDED               @\n");
    printf(BRED " @==================================@\n" RESET "\n");
    if (dedup_table) free(dedup_table);
    open_github();
    exit(0);
}

void splash_anim(void) {
    printf(CLEAR HIDE_CURSOR);
    const char *lines[] = {
        BRED  "  ####  ###  ####  #  #",
        BORA  "  #  # #   # #  # # #  ",
        BYEL  "  #  # ##### ####  ##  ",
        BGRN  "  #  # #   # #  # # #  ",
        BCYN  "  ####  #   # #  # #  #",
        NULL
    };
    const char *lines2[] = {
        RED   "   #### ####  #   #",
        ORA   "  #     #     ##  #",
        YEL   "  # ##  ###   # # #",
        GRN   "  #  #  #     #  ##",
        CYN   "   #### ####  #   #",
        NULL
    };
    for (int i = 0; lines[i]; i++) {
        printf("\n %s" RESET "  %s" RESET, lines[i], lines2[i] ? lines2[i] : "");
        fflush(stdout);
        usleep(55000);
    }
    printf("\n\n");
    printf(DGRAY "  ======================================================\n" RESET);
    printf(BTEAL "  DARK-GEN " RESET "v" VERSION);
    printf(LGRAY "  |  " RESET BWHT "SHRAVAN ACHARYA" RESET);
    printf(LGRAY "  |  " RESET DIM "Password Intelligence Suite" RESET "\n");
    printf(DGRAY "  ======================================================\n" RESET);
    printf(SHOW_CURSOR);
    usleep(600000);
}

void banner(void) {
    printf(CLEAR);
    printf("\n");
    printf(BRED  "  ####  ###  ####  #  #" RESET "  ");
    printf(RED   " #### ####  #   #\n" RESET);
    printf(BORA  "  #  # #   # #  # # #  " RESET "  ");
    printf(ORA   "#     #     ##  #\n" RESET);
    printf(BYEL  "  #  # ##### ####  ##  " RESET "  ");
    printf(YEL   "# ##  ###   # # #\n" RESET);
    printf(BGRN  "  #  # #   # #  # # #  " RESET "  ");
    printf(GRN   "#  #  #     #  ##\n" RESET);
    printf(BCYN  "  ####  #   # #  # #  #" RESET "  ");
    printf(CYN   " #### ####  #   #\n" RESET);
    printf("\n");
    printf(DGRAY "  ------------------------------------------------------\n" RESET);
    printf("  " BTEAL "v%s" RESET LGRAY " | " RESET BWHT "SHRAVAN ACHARYA" RESET LGRAY " | " RESET BGRN "github.com/techpythoncode-bit\n" RESET, VERSION);
    printf(DGRAY "  ------------------------------------------------------\n\n" RESET);
}

static const char *SPIN[] = {"@","â‚¹","_","-","&","(","*","#","$","!"};
static const char *BLOCKS[] = {"-","=","#","@"};

void progress_hud(unsigned long long current, int frame, time_t t_start) {
    unsigned long long total = total_expected ? total_expected : cfg.target;
    double ratio = (total > 0) ? (double)current / (double)total : 0.0;
    if (ratio > 1.0) ratio = 1.0;
    int pos = (int)(BAR_WIDTH * ratio);
    int pct = (int)(ratio * 100.0);
    
    time_t now = time(NULL);
    double elapsed = difftime(now, t_start);
    double target_time = 20.0;
    double adjusted_elapsed = elapsed;
    if (elapsed < target_time && current < total) {
        adjusted_elapsed = target_time * ratio;
    }
    
    double speed = (adjusted_elapsed > 0.1) ? (double)current / adjusted_elapsed : 0.0;
    double eta = (speed > 0 && current < total) ? (double)(total - current) / speed : 0.0;

    pthread_mutex_lock(&mtx);
    char snap[48];
    snprintf(snap, sizeof(snap), "%.46s", (char*)ticker);
    pthread_mutex_unlock(&mtx);

    const char *bar_col = (pct >= 80) ? BGRN : (pct >= 50) ? BYEL : (pct >= 25) ? BORA : BRED;

    printf("\r\033[K");
    printf("  %s%s" RESET " ", bar_col, SPIN[frame % 10]);
    printf(DGRAY "|" RESET);
    for (int i = 0; i < BAR_WIDTH; i++) {
        if (i < pos - 2) printf(BGRN "%s", BLOCKS[3]);
        else if (i < pos - 1) printf(BGRN "%s", BLOCKS[2]);
        else if (i < pos) printf(BYEL "%s", BLOCKS[1]);
        else printf(DGRAY "%s", BLOCKS[0]);
    }
    printf(DGRAY "|" RESET);
    printf(" %s%3d%%" RESET, bar_col, pct);

    printf("\n\033[K");
    printf("  " DGRAY "|" RESET);
    printf(" " BCYN "%-24.24s" RESET, snap);
    printf(DGRAY " |" RESET);
    printf(" " BWHT "%7llu" RESET LGRAY " pw", current);
    printf(DGRAY " |" RESET);
    if (speed >= 1000.0)
        printf(" " BMAG "%.1fk" RESET LGRAY "/s", speed / 1000.0);
    else
        printf(" " BMAG "%.0f" RESET LGRAY "/s ", speed);
    printf(DGRAY " |" RESET);
    if (eta > 0 && current < total) {
        if (eta < 60) printf(" " BYEL "ETA %2ds" RESET, (int)eta);
        else printf(" " BYEL "ETA %dm%02ds" RESET, (int)eta/60, (int)eta%60);
    } else {
        printf(" " BGRN "Done   " RESET);
    }
    if (cfg.dedup) printf(DGRAY " |" RESET RED " -%llu" RESET, g_dupes);

    printf("\033[1A");
    fflush(stdout);
    
    if (adjusted_elapsed < target_time && current < total) {
        usleep(50000);
    }
}

static void str_lower(char *s) {
    for (; *s; s++) *s = (char)tolower((unsigned char)*s);
}

static void str_upper(const char *s, char *d, size_t n) {
    size_t l = strlen(s);
    if (l >= n) l = n - 1;
    memcpy(d, s, l);
    d[l] = '\0';
    for (char *p = d; *p; p++) *p = (char)toupper((unsigned char)*p);
}

static void str_title(const char *s, char *d, size_t n) {
    size_t l = strlen(s);
    if (l >= n) l = n - 1;
    memcpy(d, s, l);
    d[l] = '\0';
    if (d[0]) d[0] = (char)toupper((unsigned char)d[0]);
}

static void leet(const char *s, char *d, size_t n) {
    size_t l = strlen(s);
    if (l >= n) l = n - 1;
    memcpy(d, s, l);
    d[l] = '\0';
    for (char *p = d; *p; p++) {
        switch (tolower((unsigned char)*p)) {
            case 'a': *p = '4'; break;
            case 'e': *p = '3'; break;
            case 'i': *p = '1'; break;
            case 'o': *p = '0'; break;
            case 's': *p = '5'; break;
            case 't': *p = '7'; break;
            case 'g': *p = '9'; break;
            case 'b': *p = '8'; break;
        }
    }
}

static void str_rev(const char *s, char *d, size_t n) {
    size_t l = strlen(s);
    if (l >= n) l = n - 1;
    for (size_t i = 0; i < l; i++) d[i] = s[l - 1 - i];
    d[l] = '\0';
}

static void update_stats(const char *pw) {
    int ha = 0, hd = 0, hs = 0;
    size_t len = strlen(pw);
    for (const char *p = pw; *p; p++) {
        if (isalpha((unsigned char)*p)) ha = 1;
        else if (isdigit((unsigned char)*p)) hd = 1;
        else hs = 1;
    }
    if (hs) __sync_fetch_and_add(&stat_special, 1);
    else if (ha && hd) __sync_fetch_and_add(&stat_mixed, 1);
    else if (ha) __sync_fetch_and_add(&stat_alpha, 1);
    else __sync_fetch_and_add(&stat_digit, 1);
    if (len < 8) __sync_fetch_and_add(&stat_short, 1);
    if (len >= 12) __sync_fetch_and_add(&stat_long, 1);
}

typedef struct {
    FILE *fp;
    gzFile gz;
    int is_gz;
} OutFile;

static OutFile open_outfile(const char *path, int compress_mode) {
    OutFile o = {NULL, NULL, 0};
    if (compress_mode) {
        o.gz = gzopen(path, "wb9");
        o.is_gz = 1;
        if (!o.gz) fprintf(stderr, BRED "\n  [ERR] Cannot open gz file: %s\n" RESET, strerror(errno));
    } else {
        o.fp = fopen(path, "w");
        if (!o.fp) fprintf(stderr, BRED "\n  [ERR] Cannot open file: %s\n" RESET, strerror(errno));
    }
    return o;
}

static int write_line_outfile(OutFile *o, const char *line) {
    if (o->is_gz) {
        if (!o->gz) return 0;
        return (gzprintf(o->gz, "%s\n", line) > 0);
    } else {
        if (!o->fp) return 0;
        return (fprintf(o->fp, "%s\n", line) > 0);
    }
}

static void close_outfile(OutFile *o) {
    if (o->is_gz && o->gz) {
        gzclose(o->gz);
        o->gz = NULL;
    }
    if (!o->is_gz && o->fp) {
        fclose(o->fp);
        o->fp = NULL;
    }
}

static int outfile_ok(OutFile *o) {
    return o->is_gz ? (o->gz != NULL) : (o->fp != NULL);
}

static inline int write_pw(OutFile *f, const char *pw, volatile unsigned long long *cnt) {
    if (*cnt >= cfg.target) return 0;
    if (cfg.dedup) {
        if (dedup_seen(pw)) {
            __sync_fetch_and_add(&g_dupes, 1);
            return 1;
        }
    }
    if (!write_line_outfile(f, pw)) return 0;
    if (cfg.show_stats) update_stats(pw);
    pthread_mutex_lock(&mtx);
    (*cnt)++;
    snprintf((char*)ticker, sizeof(ticker) - 1, "%s", pw);
    pthread_mutex_unlock(&mtx);
    return 1;
}

static const char *SUFFIXES[] = {
    "1","12","123","1234","12345","123456","!","!!","!!!","@","#","$","!@#","@#$",
    "2020","2021","2022","2023","2024","2025","786","007","666","999","000","69",
    "420","911","admin","pass","secure","god","hack","root","xo","@123","_123",
    "#1","@1","01","1!","0","00","01","99","abc","xyz"
};
static const int SUF_N = 45;

static const char *PREFIXES[] = {
    "Mr","Dr","@","#","the","my","i","its","_","!","hot","super","cool","dark","pro","x"
};
static const int PRE_N = 16;

static const char *SEPARATORS[] = {"", "_", ".", "-", "@", "#", "!"};
static const int SEP_N = 7;

static const char *YEARS[] = {
    "1990","1991","1992","1993","1994","1995","1996","1997","1998","1999",
    "2000","2001","2002","2003","2004","2005","2006","2007","2008","2009",
    "2010","2011","2012","2013","2014","2015","2016","2017","2018","2019",
    "2020","2021","2022","2023","2024","2025","2026"
};
static const int YR_N = 37;

static const char *COMMON_WORDS[] = {
    "password","123456","12345678","qwerty","admin","welcome","letmein","football",
    "soccer","baseball","basketball","monkey","dragon","shadow","superman","batman",
    "pokemon","princess","sunshine","spring","summer","winter","autumn","coffee",
    "pizza","cheese","hunter2","login","secret","trustnoone","matrix","skywalker",
    "starwars","tuesday","friday","january","password123","guest","root","user1234",
    "qwertyuiop","asdfghjkl","zxcvbnm","iloveyou","mustang","ferrari","porsche",
    "disney","mickey","donald","google","master","michael","jessica","charlie",
    "trustno1","abc123","purple","orange","thunder","hunter","ranger","panther",
    "falcon","eagle","tiger","lion","wolf","cobra","viper","ninja","samurai","warrior",
    "knight","wizard","phoenix","hacker","cyber","digital","online","cloud",
    "server","system","network","security","private","access","bypass","crack",
    "attack","exploit","naruto","anime","gaming","player","sniper",
    "god","devil","angel","demon","ghost","spirit","soul","fire","ice","black","white",
    "dark","light","death","life","night","day","blood","andrew","jennifer","daniel",
    "ashley","matthew","amanda","joshua","melissa","david","sarah","robert","lisa",
    "james","mary","john","patricia","william","linda","richard","barbara","joseph",
    "elizabeth","thomas","susan","charles","karen","christopher","nancy","steven",
    "betty","brian","margaret","edward","sandra","ronald","ashley","anthony","donna",
    "kevin","emily","jason","michelle","justin","kimberly","ryan","laura","eric",
    "stephanie","jacob","rebecca","gary","nicholas","jonathan",
    "larry","frank","scott","brandon","raymond","gregory","samuel","patrick","alexander",
    "jack","dennis","jerry","tyler","aaron","jose","adam","henry","nathan",
    "douglas","zachary","peter","kyle","walter","ethan","jeremy","harold","keith",
    "christian","roger","noah","gerald","carl","terry","sean","austin","arthur",
    "lawrence","jesse","dylan","jordan","bryan","billy","joe","logan","albert","willie",
    "alan","juan","wayne","ralph","roy","eugene","randy","vincent","russell","louis",
    "philip","bobby","johnny","bradley",
    "password1","letmein1","welcome1","monkey1","dragon1","master1","sunshine1",
    "princess1","football1","superman1","batman1","spider1","login1","admin1","qwerty1",
    "abc1234","iloveyou1","shadow1","baseball1","soccer1","hockey1","purple1","orange1",
    "thunder1","hunter1","ranger1","mustang1","panther1","falcon1","eagle1","tiger1",
    "lion1","wolf1","cobra1","viper1","ninja1","samurai1","warrior1","knight1",
    "wizard1","phoenix1","matrix1","hacker1","cyber1","digital1","online1","cloud1",
    "server1","system1","network1","security1","secret1","private1","access1","bypass1",
    "starwars1","pokemon1","naruto1","gaming1","player1","sniper1","god1","devil1",
    "angel1","demon1","ghost1","fire1","ice1","black1","white1","dark1","light1",
    "pass123","admin123","root123","user123","guest123","test123","login123","welcome123",
    "master123","super123","letmein123","qwerty123","password12","password1234","pass1234",
    "loveyou","iloveu","love123","lovely","lover","trustme","trustno","freedom","liberty",
    "justice","america","england","germany","france","russia","china","japan","india",
    "brazil","canada","mexico","spain","italy","australia","korea","vietnam","thailand",
    "turkey","poland","ukraine","romania","netherlands","belgium","greece","portugal",
    "sweden","austria","switzerland","denmark","finland","norway","ireland","newzealand",
    "singapore","malaysia","indonesia","philippines","pakistan","bangladesh","egypt",
    "nigeria","southafrica","argentina","colombia","venezuela","chile","peru","morocco",
    "january1","february1","march1","april1","may1","june1","july1","august1",
    "september1","october1","november1","december1","monday","tuesday1","wednesday",
    "thursday","friday1","saturday","sunday","hello","hello123","hello1","goodbye",
    "welcome123","welcome12","welcome1234","password!","password@","password#","password$",
    "qwerty!","qwerty@","qwerty#","admin!","admin@","admin#","admin$","root!","root@",
    "pass!","pass@","pass#","letmein!","letmein@","login!","login@","master!","master@",
    "super!","super@","super#","dragon!","dragon@","monkey!","monkey@","batman!","batman@",
    "superman!","superman@","football!","football@","baseball!","baseball@","soccer!",
    "soccer@","hockey!","hockey@","basketball!","basketball@","computer","laptop","desktop",
    "keyboard","mouse","monitor","printer","scanner","camera","phone","mobile","tablet",
    "android","iphone","samsung","nokia","motorola","sony","apple","windows","linux",
    "ubuntu","debian","fedora","centos","redhat","oracle","mysql","postgres","mongodb",
    "redis","elasticsearch","apache","nginx","tomcat","jenkins","docker","kubernetes",
    "aws","azure","gcp","cloud","devops","python","java","javascript","typescript","ruby",
    "php","csharp","cplusplus","golang","rust","swift","kotlin","scala","perl","bash",
    "powershell","cmd","terminal","console","shell","script","code","program","software",
    "hardware","network","internet","wifi","ethernet","router","modem","switch","firewall",
    "antivirus","malware","virus","trojan","worm","ransomware","phishing","spam","scam",
    "facebook","twitter","instagram","youtube","tiktok","snapchat","whatsapp","telegram",
    "discord","reddit","linkedin","github","gitlab","bitbucket","stackoverflow","medium",
    "netflix","spotify","amazon","ebay","paypal","venmo","cashapp","bitcoin","ethereum",
    "blockchain","crypto","wallet","exchange","trading","forex","stock","investment",
    "money","dollar","euro","pound","rupee","yen","won","peso","real","rand",
    "alpha","beta","gamma","delta","epsilon","zeta","eta","theta","iota","kappa",
    "lambda","sigma","omega","phoenix","dragon","griffin","unicorn","pegasus","hydra",
    "cerberus","minotaur","medusa","cyclops","titan","giant","dwarf","elf","orc",
    "goblin","troll","vampire","werewolf","zombie","ghost","demon","angel","heaven",
    "hell","paradise","inferno","purgatory","limbo","nirvana","valhalla","olympus",
    "asgard","atlantis","camelot","avalon","shangri","eldorado","utopia","dystopia",
    "genesis","exodus","revelation","prophecy","destiny","fortune","fate","karma",
    "chakra","zen","tao","yoga","meditation","enlightenment","awakening","ascension",
    "power","strength","courage","bravery","honor","glory","victory","triumph","conquest",
    "domination","supremacy","authority","command","control","influence","leadership",
    "excellence","perfection","mastery","expertise","skill","talent","genius","prodigy",
    "legend","myth","hero","champion","warrior","gladiator","soldier","marine","navy",
    "army","force","troop","squad","team","crew","gang","clan","guild","alliance", NULL
};

static void emit_variants(OutFile *f, const char *base, volatile unsigned long long *cnt) {
    if (!base || strlen(base) < 1) return;
    char lo[MAX_LEN], up[MAX_LEN], ti[MAX_LEN], lt[MAX_LEN], rv[MAX_LEN];
    strncpy(lo, base, MAX_LEN - 1);
    lo[MAX_LEN - 1] = '\0';
    str_lower(lo);
    str_upper(lo, up, MAX_LEN);
    str_title(lo, ti, MAX_LEN);
    leet(lo, lt, MAX_LEN);
    str_rev(lo, rv, MAX_LEN);
    const char *forms[] = {lo, up, ti, lt, rv, base};
    int nf = 6;
    for (int fi = 0; fi < nf && *cnt < cfg.target; fi++) write_pw(f, forms[fi], cnt);
    for (int fi = 0; fi < nf && *cnt < cfg.target; fi++)
        for (int si = 0; si < SUF_N && *cnt < cfg.target; si++) {
            char pw[512];
            snprintf(pw, sizeof(pw), "%s%s", forms[fi], SUFFIXES[si]);
            write_pw(f, pw, cnt);
        }
    for (int fi = 0; fi < nf && *cnt < cfg.target; fi++)
        for (int pi = 0; pi < PRE_N && *cnt < cfg.target; pi++) {
            char pw[512];
            snprintf(pw, sizeof(pw), "%s%s", PREFIXES[pi], forms[fi]);
            write_pw(f, pw, cnt);
        }
    for (int fi = 0; fi < nf && *cnt < cfg.target; fi++)
        for (int yi = 0; yi < YR_N && *cnt < cfg.target; yi++) {
            char pw[512];
            snprintf(pw, sizeof(pw), "%s%s", forms[fi], YEARS[yi]);
            write_pw(f, pw, cnt);
        }
}

static void emit_pairs(OutFile *f, const char *a, const char *b, volatile unsigned long long *cnt) {
    if (!a || !b || !strlen(a) || !strlen(b)) return;
    char alo[MAX_LEN], blo[MAX_LEN], ati[MAX_LEN], bti[MAX_LEN];
    strncpy(alo, a, MAX_LEN - 1);
    alo[MAX_LEN - 1] = '\0';
    str_lower(alo);
    strncpy(blo, b, MAX_LEN - 1);
    blo[MAX_LEN - 1] = '\0';
    str_lower(blo);
    str_title(alo, ati, MAX_LEN);
    str_title(blo, bti, MAX_LEN);
    const char *af[] = {alo, ati, a};
    const char *bf[] = {blo, bti, b};
    for (int ai = 0; ai < 3 && *cnt < cfg.target; ai++)
        for (int bi = 0; bi < 3 && *cnt < cfg.target; bi++)
            for (int si = 0; si < SEP_N && *cnt < cfg.target; si++) {
                char pw[512];
                snprintf(pw, sizeof(pw), "%s%s%s", af[ai], SEPARATORS[si], bf[bi]);
                write_pw(f, pw, cnt);
                for (int sfi = 0; sfi < SUF_N && *cnt < cfg.target; sfi++) {
                    snprintf(pw, sizeof(pw), "%s%s%s%s", af[ai], SEPARATORS[si], bf[bi], SUFFIXES[sfi]);
                    write_pw(f, pw, cnt);
                }
            }
}

static int bf_recurse(OutFile *f, const char *cs, int cl, char *buf, int depth, int max, volatile unsigned long long *cnt) {
    if (*cnt >= cfg.target) return 0;
    if (depth == max) {
        buf[depth] = '\0';
        return write_pw(f, buf, cnt);
    }
    for (int i = 0; i < cl && *cnt < cfg.target; i++) {
        buf[depth] = cs[i];
        if (!bf_recurse(f, cs, cl, buf, depth + 1, max, cnt)) return 0;
    }
    return 1;
}

static void emit_dates(OutFile *f, volatile unsigned long long *cnt) {
    char pw[64];
    for (int y = 1950; y <= 2026 && *cnt < cfg.target; y++)
        for (int m = 1; m <= 12 && *cnt < cfg.target; m++)
            for (int d = 1; d <= 31 && *cnt < cfg.target; d++) {
                snprintf(pw, sizeof(pw), "%02d%02d%04d", d, m, y);
                write_pw(f, pw, cnt);
                snprintf(pw, sizeof(pw), "%02d%02d%04d", m, d, y);
                write_pw(f, pw, cnt);
                snprintf(pw, sizeof(pw), "%02d%02d%02d", d, m, y % 100);
                write_pw(f, pw, cnt);
                snprintf(pw, sizeof(pw), "%04d%02d%02d", y, m, d);
                write_pw(f, pw, cnt);
            }
    for (int n = 0; n <= 9999 && *cnt < cfg.target; n++) {
        snprintf(pw, sizeof(pw), "%04d", n);
        write_pw(f, pw, cnt);
    }
    for (int n = 0; n <= 999999 && *cnt < cfg.target; n++) {
        snprintf(pw, sizeof(pw), "%06d", n);
        write_pw(f, pw, cnt);
    }
}

void *engine(void *arg) {
    gen_args *d = (gen_args *)arg;
    char final_path[520];
    snprintf(final_path, sizeof(final_path), "%s", d->file);
    char tmp_path[520];
    snprintf(tmp_path, sizeof(tmp_path), "%s" SCRATCH_EXT, d->file);

    OutFile f = open_outfile(tmp_path, cfg.compress);
    if (!outfile_ok(&f)) {
        *d->active = 0;
        return NULL;
    }

    if (d->mode == 1) {
        for (int i = 0; i < MAX_INTEL && *d->count < cfg.target; i++) {
            if (!strlen(d->intel[i]) || !strcmp(d->intel[i], "s")) continue;
            emit_variants(&f, d->intel[i], d->count);
        }
        for (int i = 0; i < MAX_INTEL && *d->count < cfg.target; i++) {
            if (!strlen(d->intel[i]) || !strcmp(d->intel[i], "s")) continue;
            for (int j = i + 1; j < MAX_INTEL && *d->count < cfg.target; j++) {
                if (!strlen(d->intel[j]) || !strcmp(d->intel[j], "s")) continue;
                emit_pairs(&f, d->intel[i], d->intel[j], d->count);
            }
        }
        if (strlen(d->custom) >= 1 && strcmp(d->custom, "s")) {
            for (int i = 0; i < MAX_INTEL && *d->count < cfg.target; i++) {
                if (!strlen(d->intel[i]) || !strcmp(d->intel[i], "s")) continue;
                emit_pairs(&f, d->intel[i], d->custom, d->count);
            }
            emit_variants(&f, d->custom, d->count);
        }
        for (int i = 0; COMMON_WORDS[i] && *d->count < cfg.target; i++) emit_variants(&f, COMMON_WORDS[i], d->count);
    } else if (d->mode == 2) {
        for (int i = 0; i < MAX_INTEL && *d->count < cfg.target; i++) {
            if (!strlen(d->intel[i]) || !strcmp(d->intel[i], "s")) continue;
            emit_variants(&f, d->intel[i], d->count);
        }
        for (int i = 0; i < MAX_INTEL && *d->count < cfg.target; i++) {
            if (!strlen(d->intel[i]) || !strcmp(d->intel[i], "s")) continue;
            for (int j = 0; COMMON_WORDS[j] && *d->count < cfg.target; j++) emit_pairs(&f, d->intel[i], COMMON_WORDS[j], d->count);
        }
        for (int i = 0; COMMON_WORDS[i] && *d->count < cfg.target; i++) emit_variants(&f, COMMON_WORDS[i], d->count);
    } else if (d->mode == 3) {
        static const char *god_seeds[] = {
            "admin","root","user","guest","test","pass","password","login","access","secure",
            "system","server","network","master","backup","default","public","private","super",
            "god","elite","hack","cyber","shadow","ghost","dark","black","white","red","blue",
            "fire","ice","storm","rock","iron","steel","gold","silver","diamond","power","force",
            "blade","doom","void","zero","alpha","beta","omega","delta","sigma","apex","prime",
            "ultra","mega","hyper","turbo","nitro","max","boss","king","queen","ace","pro","star",
            "hero","legend","myth","core","code","data","byte","bit","sync","flux","grid","node",
            "port","love","hate","pain","rage","fear","hope","fate","war","peace","truth", NULL
        };
        for (int i = 0; god_seeds[i] && *d->count < cfg.target; i++) emit_variants(&f, god_seeds[i], d->count);
        for (int i = 0; god_seeds[i] && *d->count < cfg.target; i++)
            for (int j = i + 1; god_seeds[j] && *d->count < cfg.target; j++) emit_pairs(&f, god_seeds[i], god_seeds[j], d->count);
        for (int i = 0; COMMON_WORDS[i] && *d->count < cfg.target; i++) emit_variants(&f, COMMON_WORDS[i], d->count);
    } else if (d->mode == 4) {
        const char *cs = strlen(d->charset) ? d->charset : "abcdefghijklmnopqrstuvwxyz0123456789";
        int cl = (int)strlen(cs);
        char buf[64];
        for (int len = d->bf_min; len <= d->bf_max && *d->count < cfg.target; len++) bf_recurse(&f, cs, cl, buf, 0, len, d->count);
    } else if (d->mode == 5) {
        FILE *imp = fopen(d->import, "r");
        if (!imp) fprintf(stderr, BRED "\n  [ERR] Cannot open import file.\n" RESET);
        else {
            char line[MAX_LEN];
            while (fgets(line, sizeof(line), imp) && *d->count < cfg.target) {
                line[strcspn(line, "\r\n")] = '\0';
                if (!strlen(line)) continue;
                emit_variants(&f, line, d->count);
            }
            fclose(imp);
        }
    } else if (d->mode == 6) {
        emit_dates(&f, d->count);
    }

    close_outfile(&f);

    if (rename(tmp_path, final_path) != 0) {
        fprintf(stderr, BRED "\n  [ERR] File rename failed: %s - output kept as %s\n" RESET, strerror(errno), tmp_path);
    }

    *d->active = 0;
    return NULL;
}

static void flush_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static void read_field(const char *pr, char *buf, int max) {
    printf("%s", pr);
    fflush(stdout);
    if (!fgets(buf, max, stdin)) buf[0] = '\0';
    buf[strcspn(buf, "\n")] = '\0';
    if (!strlen(buf)) strcpy(buf, "s");
}

static unsigned long long ask_pw_count(void) {
    unsigned long long n = 0;
    for (;;) {
        printf("\n");
        printf(BMAG "  @====================================================@\n");
        printf(BMAG "  @  " BWHT "**  How many passwords to generate?            " BMAG "@\n");
        printf(BMAG "  @  " LGRAY "   Range: " BYEL "%d" LGRAY " - " BYEL "%d" LGRAY "  |  0 = default (%llu)" BMAG "  @\n" RESET, MIN_LIMIT, MAX_LIMIT, cfg.target);
        printf(BMAG "  @====================================================@\n" RESET);
        printf(BCYN "   @ Count: " RESET);
        fflush(stdout);
        if (scanf("%llu", &n) != 1) {
            flush_stdin();
            n = 0;
            break;
        }
        flush_stdin();
        if (n == 0) break;
        if (n < MIN_LIMIT) {
            printf(BRED "  [!] Minimum is %d. Try again.\n" RESET, MIN_LIMIT);
            continue;
        }
        if (n > MAX_LIMIT) {
            printf(BYEL "  [!] Capped at %d.\n" RESET, MAX_LIMIT);
            n = MAX_LIMIT;
            break;
        }
        break;
    }
    return (n == 0) ? cfg.target : n;
}

void settings_menu(void) {
    banner();
    printf(BGRN "  @====================================================@\n");
    printf(BGRN "  @  " BOLD "**  CONFIGURATION PANEL" RESET BGRN "                          @\n");
    printf(BGRN "  @=========@==========================================@\n" RESET);
    printf(BGRN "  @ " BCYN " 1 " BGRN "@ " BWHT "Output limit   " LGRAY "(default: " BYEL "%llu" LGRAY ")       " BGRN "@\n" RESET, cfg.target);
    printf(BGRN "  @ " BCYN " 2 " BGRN "@ " BWHT "Dedup filter   " LGRAY "(current: " BYEL "%s" LGRAY ")        " BGRN "@\n" RESET, cfg.dedup ? BGRN "ON " : BRED "OFF");
    printf(BGRN "  @ " BCYN " 3 " BGRN "@ " BWHT "GZip compress  " LGRAY "(current: " BYEL "%s" LGRAY ")        " BGRN "@\n" RESET, cfg.compress ? BGRN "ON " : BRED "OFF");
    printf(BGRN "  @ " BCYN " 4 " BGRN "@ " BWHT "Stats report   " LGRAY "(current: " BYEL "%s" LGRAY ")        " BGRN "@\n" RESET, cfg.show_stats ? BGRN "ON " : BRED "OFF");
    printf(BGRN "  @ " BCYN " 0 " BGRN "@ " BWHT "Back to menu" BGRN "                            @\n");
    printf(BGRN "  @=========@==========================================@\n" RESET);
    printf(BCYN "\n   @ CHOICE: " RESET);
    fflush(stdout);
    int c;
    if (scanf("%d", &c) != 1) {
        flush_stdin();
        return;
    }
    flush_stdin();
    switch (c) {
        case 1: {
            printf(BYEL "  Limit (%d-%d): " RESET, MIN_LIMIT, MAX_LIMIT);
            fflush(stdout);
            unsigned long long v;
            if (scanf("%llu", &v) == 1 && v >= (unsigned)MIN_LIMIT && v <= (unsigned)MAX_LIMIT) cfg.target = v;
            flush_stdin();
            break;
        }
        case 2:
            cfg.dedup = !cfg.dedup;
            printf(BGRN "  [*] Dedup %s\n" RESET, cfg.dedup ? "enabled" : "disabled");
            usleep(600000);
            break;
        case 3:
            cfg.compress = !cfg.compress;
            printf(BGRN "  [*] GZip compression %s\n" RESET, cfg.compress ? "enabled" : "disabled");
            usleep(600000);
            break;
        case 4:
            cfg.show_stats = !cfg.show_stats;
            printf(BGRN "  [*] Stats %s\n" RESET, cfg.show_stats ? "enabled" : "disabled");
            usleep(600000);
            break;
        default:
            break;
    }
}

void tool_flow(int mode) {
    char intel[MAX_INTEL][MAX_LEN];
    memset(intel, 0, sizeof(intel));
    char custom[MAX_LEN] = "s", file[512] = "darkgen_output";
    char import_path[512] = "", charset[256] = "";
    int bf_min = 3, bf_max = 5;
    volatile unsigned long long count = 0;
    volatile int active = 1;
    stat_alpha = stat_digit = stat_mixed = stat_special = stat_short = stat_long = g_dupes = 0;

    const char *pr[MAX_INTEL] = {
        "First Name","Last Name","Partner Name","Anniversary (ddmmyyyy)","Partner DOB",
        "Birth Year","Pet Name","Favourite Food","Child Name","Old Password",
        "Mother Name","Father Name","Middle Name","Favourite City","High School",
        "Best Friend","Favourite Hobby","Car Brand/Model","Company/Workplace","Secret Word"
    };

    banner();
    flush_stdin();

    unsigned long long pw_count = ask_pw_count();
    cfg.target = pw_count;

    if (mode == 1 || mode == 2) {
        printf("\n");
        if (mode == 1)
            printf(BGRN "  @====================================================@\n"
                   "  @  **  CUSTOM TARGET PROFILER                       @\n"
                   "  @      Enter info below - type " BYEL "'s'" BGRN " to skip a field @\n"
                   "  @====================================================@\n\n" RESET);
        else
            printf(BMAG "  @====================================================@\n"
                   "  @  **  INTEL + COMMON MERGE                         @\n"
                   "  @      Enter info below - type " BYEL "'s'" BMAG " to skip a field @\n"
                   "  @====================================================@\n\n" RESET);
        for (int i = 0; i < MAX_INTEL; i++) {
            char prompt[180];
            snprintf(prompt, sizeof(prompt), "  " LGRAY "[" BYEL "%02d/%02d" LGRAY "] " BWHT "%-22s " LGRAY ">> " RESET, i + 1, MAX_INTEL, pr[i]);
            read_field(prompt, intel[i], MAX_LEN);
        }
        printf(BCYN "\n  [+] " RESET "Custom injection word (or 's' to skip): ");
        read_field("", custom, MAX_LEN);
    }
    if (mode == 3) {
        printf(BRED "\n  @====================================================@\n"
               "  @  **  GOD-LIST GENERATOR                           @\n"
               "  @====================================================@\n\n" RESET);
    }
    if (mode == 4) {
        printf(BBLU "\n  @====================================================@\n"
               "  @  **  CHARSET BRUTE-FORCE GENERATOR                @\n"
               "  @====================================================@\n\n" RESET);
        printf(BCYN "  [CS] " RESET "Charset (Enter for lowercase+digits): ");
        fflush(stdout);
        if (fgets(charset, sizeof(charset), stdin)) charset[strcspn(charset, "\n")] = '\0';
        printf(BCYN "  [MIN] " RESET "Min length (1-8): ");
        fflush(stdout);
        if (scanf("%d", &bf_min) != 1) bf_min = 3;
        flush_stdin();
        printf(BCYN "  [MAX] " RESET "Max length (%d-8): ", bf_min);
        fflush(stdout);
        if (scanf("%d", &bf_max) != 1) bf_max = 5;
        flush_stdin();
    }
    if (mode == 5) {
        printf(BGRN "\n  @====================================================@\n"
               "  @  **  IMPORT WORDLIST + MUTATE                     @\n"
               "  @====================================================@\n\n" RESET);
        printf(BCYN "  [IN] " RESET "Path to wordlist file: ");
        read_field("", import_path, sizeof(import_path));
    }
    if (mode == 6) {
        printf(BYEL "\n  @====================================================@\n"
               "  @  **  DATE / PIN BURST GENERATOR                   @\n"
               "  @====================================================@\n\n" RESET);
    }

    if (cfg.dedup) {
        dedup_table = (uint32_t*)calloc(HASH_SIZE, sizeof(uint32_t));
        if (!dedup_table) {
            printf(BRED "  [!] Memory Error. Dedup disabled.\n" RESET);
            cfg.dedup = 0;
        } else {
            printf(BGRN "  [*] Dedup filter active (32 MB hash)\n" RESET);
            usleep(400000);
        }
    }

    printf(BCYN "\n  [OUT] " RESET "Output name (Enter = '%s'): ", file);
    fflush(stdout);
    char tmp[512];
    if (fgets(tmp, sizeof(tmp), stdin)) {
        tmp[strcspn(tmp, "\n")] = '\0';
        if (strlen(tmp)) {
            strncpy(file, tmp, sizeof(file) - 10);
            file[sizeof(file) - 10] = '\0';
        }
    }
    char *dot = strrchr(file, '.');
    if (dot && (!strcmp(dot, ".txt") || !strcmp(dot, ".gz"))) *dot = '\0';
    if (cfg.compress) strncat(file, ".txt.gz", 8);
    else strncat(file, ".txt", 5);

    static const char *mnames[] = {"","Custom Profile","Intel+Common","GOD-LIST","Charset BruteForce","Import+Mutate","Date/PIN Burst"};
    printf("\n");
    printf(DGRAY "  +-----------------------------------------------------+\n" RESET);
    printf(DGRAY "  | " BWHT "%-14s" RESET LGRAY ": " BYEL "%-36s" DGRAY "|\n" RESET, "Output", file);
    printf(DGRAY "  | " BWHT "%-14s" RESET LGRAY ": " BYEL "%-36llu" DGRAY "|\n" RESET, "Passwords", cfg.target);
    printf(DGRAY "  | " BWHT "%-14s" RESET LGRAY ": " BYEL "%-36s" DGRAY "|\n" RESET, "Mode", mnames[mode]);
    printf(DGRAY "  | " BWHT "%-14s" RESET LGRAY ": " BYEL "%-36s" DGRAY "|\n" RESET, "Dedup", cfg.dedup ? BGRN "YES" : BRED "NO");
    printf(DGRAY "  | " BWHT "%-14s" RESET LGRAY ": " BYEL "%-36s" DGRAY "|\n" RESET, "Compress", cfg.compress ? BGRN "GZip (.gz)" : BRED "None");
    printf(DGRAY "  +-----------------------------------------------------+\n" RESET);
    printf(BYEL "\n  [*] Press ENTER to start generation..." RESET);
    fflush(stdout);
    getchar();

    total_expected = cfg.target;
    gen_args d;
    memset(&d, 0, sizeof(d));
    memcpy(d.intel, intel, sizeof(intel));
    snprintf(d.custom, MAX_LEN, "%s", custom);
    snprintf(d.file, sizeof(d.file), "%s", file);
    snprintf(d.import, sizeof(d.import), "%s", import_path);
    snprintf(d.charset, sizeof(d.charset), "%s", charset);
    d.bf_min = bf_min;
    d.bf_max = bf_max;
    d.count = &count;
    d.active = &active;
    d.mode = mode;

    pthread_t t;
    pthread_create(&t, NULL, engine, &d);

    printf(HIDE_CURSOR "\n\n\n");
    int frame = 0;
    time_t t_start = time(NULL);
    while (active || count < (unsigned long long)cfg.target) {
        progress_hud(count, frame++, t_start);
        if (!active) break;
    }
    pthread_join(t, NULL);
    progress_hud(count, 0, t_start);
    printf("\n\n\n" SHOW_CURSOR);

    time_t elapsed = time(NULL) - t_start;
    struct stat st;
    long long fsz = -1;
    if (stat(file, &st) == 0) fsz = (long long)st.st_size;

    printf(BGRN "  @====================================================@\n");
    printf(BGRN "  @  **  GENERATION COMPLETE                         @\n");
    printf(BGRN "  @====================================================@\n" RESET);
    printf(LGRAY "  @ " BWHT "Total Generated  " RESET ": " BYEL "%-32llu" LGRAY "@\n" RESET, count);
    if (cfg.dedup) printf(LGRAY "  @ " BWHT "Duplicates Skip  " RESET ": " RED "%-32llu" LGRAY "@\n" RESET, g_dupes);
    printf(LGRAY "  @ " BWHT "Output File      " RESET ": " BCYN "%-32s" LGRAY "@\n" RESET, file);
    if (fsz >= 0) {
        char size_str[32];
        if (fsz >= 1048576) snprintf(size_str, sizeof(size_str), "%.2f MB", (double)fsz / 1048576.0);
        else snprintf(size_str, sizeof(size_str), "%.1f KB", (double)fsz / 1024.0);
        printf(LGRAY "  @ " BWHT "File Size        " RESET ": " BMAG "%-32s" LGRAY "@\n" RESET, size_str);
    }
    printf(LGRAY "  @ " BWHT "Time Elapsed     " RESET ": " BMAG "%-29llds" LGRAY "@\n" RESET, (long long)elapsed);
    printf(LGRAY "  @ " BWHT "Compression      " RESET ": " BMAG "%-32s" LGRAY "@\n" RESET, cfg.compress ? "GZip (level 9)" : "None");

    if (cfg.show_stats && count > 0) {
        printf(LGRAY "  @====================================================@\n" RESET);
        printf(LGRAY "  @  " BOLD "PASSWORD COMPOSITION STATS" RESET LGRAY "                        @\n");
        printf(LGRAY "  @ " RESET "Alpha-only  : " BYEL "%-8llu" LGRAY " (%.1f%%)" LGRAY "                  @\n" RESET, stat_alpha, 100.0 * stat_alpha / count);
        printf(LGRAY "  @ " RESET "Digit-only  : " BYEL "%-8llu" LGRAY " (%.1f%%)" LGRAY "                  @\n" RESET, stat_digit, 100.0 * stat_digit / count);
        printf(LGRAY "  @ " RESET "Mixed       : " BYEL "%-8llu" LGRAY " (%.1f%%)" LGRAY "                  @\n" RESET, stat_mixed, 100.0 * stat_mixed / count);
        printf(LGRAY "  @ " RESET "Special     : " BYEL "%-8llu" LGRAY " (%.1f%%)" LGRAY "                  @\n" RESET, stat_special, 100.0 * stat_special / count);
        printf(LGRAY "  @ " RESET "Short (<8)  : " BRED "%-8llu" LGRAY " (%.1f%%)" LGRAY "                  @\n" RESET, stat_short, 100.0 * stat_short / count);
        printf(LGRAY "  @ " RESET "Long  (>=12) : " BGRN "%-8llu" LGRAY " (%.1f%%)" LGRAY "                  @\n" RESET, stat_long, 100.0 * stat_long / count);
    }
    printf(LGRAY "  @====================================================@\n" RESET);

    if (dedup_table) {
        free(dedup_table);
        dedup_table = NULL;
        cfg.dedup = 1;
    }
    printf(BYEL "\n  [!] Press ENTER to return to menu..." RESET);
    fflush(stdout);
    getchar();
}

void draw_menu(void) {
    banner();
    printf(BYEL "  @==========@===========================================@\n");
    printf(BYEL "  @ " BCYN "  1  " BYEL "@ " BGRN " **  CUSTOM TARGET PROFILING                " BYEL "@\n");
    printf(BYEL "  @ " BCYN "  2  " BYEL "@ " BMAG " **  INTELLIGENCE + COMMON MERGE            " BYEL "@\n");
    printf(BYEL "  @ " BCYN "  3  " BYEL "@ " BRED " **  GENERATE GOD-LIST  (max 150k)          " BYEL "@\n");
    printf(BYEL "  @ " BCYN "  4  " BYEL "@ " BBLU " **  CHARSET BRUTE-FORCE                    " BYEL "@\n");
    printf(BYEL "  @ " BCYN "  5  " BYEL "@ " BGRN " **  IMPORT WORDLIST + MUTATE               " BYEL "@\n");
    printf(BYEL "  @ " BCYN "  6  " BYEL "@ " BCYN " **  DATE / PIN BURST GENERATOR             " BYEL "@\n");
    printf(BYEL "  @==========@===========================================@\n");
    printf(BYEL "  @ " BCYN "  7  " BYEL "@ " BMAG " **   SETTINGS / CONFIG                      " BYEL "@\n");
    printf(BYEL "  @ " BCYN "  8  " BYEL "@ " BWHT " **   ABOUT                                  " BYEL "@\n");
    printf(BYEL "  @ " BCYN "  9  " BYEL "@ " BRED " **   EXIT                                   " BYEL "@\n");
    printf(BYEL "  @==========@===========================================@\n" RESET);

    printf("\n  " DGRAY "[ " RESET);
    printf("target " BYEL "%llu" RESET, cfg.target);
    printf(DGRAY " | " RESET);
    printf("dedup " "%s" RESET, cfg.dedup ? BGRN "ON" : BRED "OFF");
    printf(DGRAY " | " RESET);
    printf("gzip " "%s" RESET, cfg.compress ? BGRN "ON" : BRED "OFF");
    printf(DGRAY " | " RESET);
    printf("stats " "%s" RESET, cfg.show_stats ? BGRN "ON" : BRED "OFF");
    printf(DGRAY " ]\n" RESET);

    printf(BCYN "\n   @ ACTION: " RESET);
    fflush(stdout);
}

void about(void) {
    banner();
    printf(BWHT "\n  DARK-GEN v" VERSION "\n" RESET);
    printf(LGRAY "  Password wordlist generator for security research\n\n" RESET);
    
    printf(BGRN "  DEVELOPER INFO:\n" RESET);
    printf(LGRAY "  Name      : " BWHT "Shravan Acharya\n" RESET);
    printf(LGRAY "  Age       : " BWHT "13 years\n" RESET);
    printf(LGRAY "  Interests : " BWHT "Python, C, C++, Bash, Shell, Ethical Hacking\n" RESET);
    printf(LGRAY "  Email     : " BCYN "techpythoncode@gmail.com\n" RESET);
    printf(LGRAY "  GitHub    : " BGRN "https://github.com/techpythoncode-bit\n\n" RESET);
    
    printf(BYEL "  ALGORITHMS:\n" RESET);
    printf(LGRAY "  - Combinatorial Password Generation\n");
    printf(LGRAY "  - FNV-1a Hash-based Deduplication\n");
    printf(LGRAY "  - Case Transformation & Leet Speak Mutations\n");
    printf(LGRAY "  - Pattern-based Word Combination (Prefixes/Suffixes)\n");
    printf(LGRAY "  - Date & PIN Permutation Generation\n");
    printf(LGRAY "  - Recursive Brute-force with Charset Customization\n\n" RESET);
    
    printf(BYEL "  [!] Press ENTER to return..." RESET);
    fflush(stdout);
    flush_stdin();
    getchar();
}

int main(void) {
    signal(SIGINT, handle_sig);
    srand((unsigned)time(NULL));
    splash_anim();
    while (1) {
        draw_menu();
        int choice;
        if (scanf("%d", &choice) != 1) {
            flush_stdin();
            continue;
        }
        flush_stdin();
        switch (choice) {
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
                tool_flow(choice);
                break;
            case 7:
                settings_menu();
                break;
            case 8:
                about();
                break;
            case 9:
                handle_sig(0);
                break;
            default:
                printf(BRED "\n  [!] Invalid choice.\n" RESET);
                usleep(600000);
        }
    }
    return 0;
}
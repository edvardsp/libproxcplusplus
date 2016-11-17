
#ifndef PROXC_H__
#define PROXC_H__

#include <stddef.h>
#include <stdint.h>

#define PROXC_NULL  ((void *)-1)

typedef void (*ProcFxn)(void);

typedef struct Chan Chan;
typedef struct Builder Builder;
typedef struct Guard Guard;

void proxc_start(ProcFxn fxn);
void proxc_exit(void);

void* proxc_argn(size_t n);
void  proxc_yield(void);

void  proxc_sleep(uint64_t usec);

Builder* proxc_proc(ProcFxn, ...);
Builder* proxc_par(int, ...);
Builder* proxc_seq(int, ...);

int proxc_go(Builder *root);
int proxc_run(Builder *root);

Guard* proxc_guardchan(int cond, Chan* chan, void *out, size_t size);
Guard* proxc_guardtime(int cond, uint64_t usec);
Guard* proxc_guardskip(int cond);
int    proxc_alt(int, ...);

Chan* proxc_chopen(size_t size);
void  proxc_chclose(Chan *chan);
int   proxc_chwrite(Chan *chan, void *data, size_t size);
int   proxc_chread(Chan *chan, void *data, size_t size);

#ifndef PROXC_NO_MACRO

#   define ARGN(index)  proxc_argn(index)
#   define YIELD()      proxc_yield()

#   define SEC(sec)     MSEC(1000 * (sec))
#   define MSEC(msec)   USEC(1000 * (msec))
#   define USEC(usec)   (usec)
#   define SLEEP(usec)  proxc_sleep(usec)

#   define PROC(...)  proxc_proc(__VA_ARGS__, PROXC_NULL)
#   define PAR(...)   proxc_par(0, __VA_ARGS__, PROXC_NULL)
#   define SEQ(...)   proxc_seq(0, __VA_ARGS__, PROXC_NULL)

#   define GO(build)   proxc_go(build)
#   define RUN(build)  proxc_run(build)

#   define CHAN_GUARD(cond, ch, out, type)  proxc_guardchan(cond, ch, out, sizeof(type))
#   define TIME_GUARD(cond, usec)           proxc_guardtime(cond, usec)
#   define SKIP_GUARD(cond)                 proxc_guardskip(cond)
#   define ALT(...)                         proxc_alt(0, __VA_ARGS__, PROXC_NULL)

#   define CHOPEN(type)               proxc_chopen(sizeof(type))
#   define CHCLOSE(chan)              proxc_chclose(chan)
#   define CHWRITE(chan, data, type)  proxc_chwrite(chan, data, sizeof(type))
#   define CHREAD(chan, data, type)   proxc_chread(chan, data, sizeof(type)) 

#endif /* PROXC_NO_MACRO */

#endif /* PROXC_H__ */


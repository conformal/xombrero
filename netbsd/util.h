#define RB_FOREACH(x, name, head)                                       \
        for ((x) = RB_MIN(name, head);                                  \
             (x) != NULL;                                               \
             (x) = name##_RB_NEXT(x))

#define RB_FOREACH_SAFE(x, name, head, y)                               \
        for ((x) = RB_MIN(name, head);                                  \
            ((x) != NULL) && ((y) = name##_RB_NEXT(x), 1);              \
             (x) = (y))

#define RB_FOREACH_REVERSE(x, name, head)                               \
        for ((x) = RB_MAX(name, head);                                  \
             (x) != NULL;                                               \
             (x) = name##_RB_PREV(x))

#define RB_FOREACH_REVERSE_SAFE(x, name, head, y)                       \
        for ((x) = RB_MAX(name, head);                                  \
            ((x) != NULL) && ((y) = name##_RB_PREV(x), 1);              \
             (x) = (y))


#ifndef TAILQ_END
#define TAILQ_END(head)                 NULL
#endif

#ifndef TAILQ_FOREACH_SAFE
#define TAILQ_FOREACH_SAFE(var, head, field, tvar)                      \
        for ((var) = TAILQ_FIRST(head);                                 \
            (var) != TAILQ_END(head) &&                                 \
            ((tvar) = TAILQ_NEXT(var, field), 1);                       \
            (var) = (tvar))
#endif

#define FMT_SCALED_STRSIZE      7       /* minus sign, 4 digits, suffix, null byte */
int     fmt_scaled(long long number, char *result);

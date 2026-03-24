#ifndef MDCAD_MATH_VALIDATE_H
#define MDCAD_MATH_VALIDATE_H

#include <stdbool.h>
#include <stdio.h>

typedef struct { int checks_run; int checks_failed; } mdcad_validation_report_t;

static inline void mdcad_validation_expect(mdcad_validation_report_t *report,
                                           bool condition,
                                           const char *label) {
    if (!report) {
        return;
    }

    report->checks_run++;
    if (!condition) {
        report->checks_failed++;
        if (label && label[0] != '\0') {
            fprintf(stderr, "VALIDATION FAIL %s\n", label);
        }
    }
}

static inline int mdcad_validation_exit_code(const mdcad_validation_report_t *report) {
    if (!report) {
        return 1;
    }
    return report->checks_failed == 0 ? 0 : 1;
}

#endif

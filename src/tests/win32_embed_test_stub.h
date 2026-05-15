#ifndef MDCAD_TESTS_WIN32_EMBED_TEST_STUB_H
#define MDCAD_TESTS_WIN32_EMBED_TEST_STUB_H

#if defined(_WIN32)
#include "../platform/win32_embed.h"
mdcad_win32_embed_state_t g_mdcad_win32_embed_state = {0};
#endif

#endif // MDCAD_TESTS_WIN32_EMBED_TEST_STUB_H

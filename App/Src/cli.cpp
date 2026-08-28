#include "cli.hpp"

#include "main.h"
#include "scenario.hpp"
#include "synth.hpp"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace cli {
namespace {

constexpr size_t kLineMax = 64;
constexpr size_t kRxMax   = 128;

uint8_t g_rx_byte;
uint8_t g_rx_ring[kRxMax];
volatile uint16_t g_rx_head;
volatile uint16_t g_rx_tail;
char g_line[kLineMax];
unsigned g_line_len;
bool g_discarding;

int g_snr_db = 99;
int g_level_db;

void write_raw(const char *s, uint16_t n)
{
	(void)HAL_UART_Transmit(&huart2, reinterpret_cast<uint8_t *>(const_cast<char *>(s)),
	                        n, 50);
}

void write(const char *s)
{
	write_raw(s, static_cast<uint16_t>(std::strlen(s)));
}

void prompt()
{
	write("bt> ");
}

void ok(const char *extra)
{
	write("ok");
	if (extra != nullptr && extra[0] != '\0') {
		write(" ");
		write(extra);
	}
	write("\r\n");
}

void err(const char *msg)
{
	write("err ");
	write(msg);
	write("\r\n");
}

int db_to_q12(int db)
{
	int32_t g = 4096;
	if (db >= 0) {
		return 4096;
	}
	int n = -db;
	while (n-- > 0) {
		g = (g * 3650) >> 12;
	}
	return static_cast<int>(g);
}

void apply_level_snr()
{
	const int32_t level = db_to_q12(g_level_db);
	synth::set_level_q12(level);

	/* PLAN section 6: every scenario opens with a noise-only preamble, so the
	   detector has a floor to seed on before any content arrives. The bench
	   floor runs during the preamble in every scenario. Scenario A carries it
	   all the way through, because noise is what A is made of. */
	const int32_t floor_q15 = (32 * level) >> 12;
	synth::set_floor_q15(floor_q15);

	if (g_snr_db >= 99) {
		synth::set_noise_q15(
			(scenario::id() == scenario::Id::A) ? floor_q15 : 0);
		return;
	}

	const int32_t rms = (synth::kTonePeak * db_to_q12(-g_snr_db)) >> 12;
	int32_t nq15 = (rms * 32767) / 2047;
	if (nq15 > 32767) {
		nq15 = 32767;
	}
	synth::set_noise_q15(nq15);
}

bool ring_push(uint8_t b)
{
	const uint16_t next = static_cast<uint16_t>((g_rx_head + 1u) % kRxMax);
	if (next == g_rx_tail) {
		return false;
	}
	g_rx_ring[g_rx_head] = b;
	g_rx_head = next;
	return true;
}

bool ring_pop(uint8_t &out)
{
	if (g_rx_tail == g_rx_head) {
		return false;
	}
	out = g_rx_ring[g_rx_tail];
	g_rx_tail = static_cast<uint16_t>((g_rx_tail + 1u) % kRxMax);
	return true;
}

void clear_rx_errors()
{
	if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_ORE) != RESET) {
		__HAL_UART_CLEAR_FLAG(&huart2, UART_CLEAR_OREF);
	}
	if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_FE) != RESET) {
		__HAL_UART_CLEAR_FLAG(&huart2, UART_CLEAR_FEF);
	}
	if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_NE) != RESET) {
		__HAL_UART_CLEAR_FLAG(&huart2, UART_CLEAR_NEF);
	}
}

void arm_rx()
{
	(void)HAL_UART_Receive_IT(&huart2, &g_rx_byte, 1);
}

void cmd_help()
{
	write("commands:\r\n");
	write("  help              this list\r\n");
	write("  scn [name]        omit to read\r\n");
	for (const scenario::Info &e : scenario::catalog()) {
		char row[80];
		if (e.alias[0] != '\0') {
			(void)std::snprintf(row, sizeof row, "    %-9s (%s)  %s\r\n",
			                    e.name, e.alias, e.blurb);
		} else {
			(void)std::snprintf(row, sizeof row, "    %-9s      %s\r\n",
			                    e.name, e.blurb);
		}
		write(row);
	}
	write("  level [dB]        0..-80  (omit to read)\r\n");
	write("  snr [dB|off]      omit to read\r\n");
	write("  run               start scenario\r\n");
	write("  stop              stop\r\n");
	write("  sync              sample count and PB0\r\n");
	write("ok: help\r\n");
}

void cmd_scn(char *arg)
{
	if (arg == nullptr || arg[0] == '\0') {
		ok(scenario::name());
		return;
	}
	if (synth::running()) {
		synth::stop();
	}
	if (!scenario::select(arg)) {
		err("unknown scenario");
		return;
	}
	apply_level_snr();
	ok(scenario::name());
}

void cmd_level(char *arg)
{
	if (arg == nullptr || arg[0] == '\0') {
		char buf[16];
		(void)std::snprintf(buf, sizeof buf, "%d", g_level_db);
		ok(buf);
		return;
	}
	g_level_db = std::atoi(arg);
	if (g_level_db > 0) {
		g_level_db = 0;
	}
	if (g_level_db < -80) {
		g_level_db = -80;
	}
	apply_level_snr();
	ok(arg);
}

void cmd_snr(char *arg)
{
	if (arg == nullptr || arg[0] == '\0') {
		if (g_snr_db >= 99) {
			ok("off");
		} else {
			char buf[16];
			(void)std::snprintf(buf, sizeof buf, "%d", g_snr_db);
			ok(buf);
		}
		return;
	}
	if (std::strcmp(arg, "off") == 0 || std::strcmp(arg, "inf") == 0) {
		g_snr_db = 99;
	} else {
		g_snr_db = std::atoi(arg);
		if (g_snr_db > 90) {
			g_snr_db = 99;
		}
		if (g_snr_db < -20) {
			g_snr_db = -20;
		}
	}
	apply_level_snr();
	ok(arg);
}

void cmd_run()
{
	if (scenario::needs_table() && !scenario::table_ready()) {
		err("no wave table");
		return;
	}
	apply_level_snr();
	if (!synth::start()) {
		err("start failed");
		return;
	}
	char buf[48];
	(void)std::snprintf(buf, sizeof buf, "run %s %lu ms", scenario::name(),
	                    static_cast<unsigned long>(scenario::duration() / 1000u));
	ok(buf);
}

void cmd_stop()
{
	synth::stop();
	ok("");
}

void cmd_sync()
{
	char buf[48];
	(void)std::snprintf(buf, sizeof buf, "sample=%lu line=%d",
	                    static_cast<unsigned long>(synth::sample_pos()),
	                    synth::sync_line() ? 1 : 0);
	ok(buf);
}

void trim_trail(char *s)
{
	if (s == nullptr) {
		return;
	}
	char *end = s + std::strlen(s);
	while (end > s && end[-1] == ' ') {
		*--end = '\0';
	}
}

void handle_line(char *line)
{
	char *cmd = line;
	while (*cmd == ' ') {
		cmd++;
	}
	trim_trail(cmd);
	if (*cmd == '\0') {
		return;
	}
	char *arg = std::strchr(cmd, ' ');
	if (arg != nullptr) {
		*arg++ = '\0';
		while (*arg == ' ') {
			arg++;
		}
		trim_trail(arg);
	}

	if (std::strcmp(cmd, "help") == 0 || std::strcmp(cmd, "?") == 0) {
		cmd_help();
	} else if (std::strcmp(cmd, "scn") == 0) {
		cmd_scn(arg);
	} else if (std::strcmp(cmd, "level") == 0) {
		cmd_level(arg);
	} else if (std::strcmp(cmd, "snr") == 0) {
		cmd_snr(arg);
	} else if (std::strcmp(cmd, "run") == 0) {
		cmd_run();
	} else if (std::strcmp(cmd, "stop") == 0) {
		cmd_stop();
	} else if (std::strcmp(cmd, "sync") == 0) {
		cmd_sync();
	} else {
		err("unknown command");
	}
}

/* Same line rules as BatV1: echo, CR or LF ends a line, backspace. */
void consume_byte(uint8_t b)
{
	switch (b) {
	case '\r':
	case '\n':
		if (g_discarding) {
			g_discarding = false;
			g_line_len = 0;
			prompt();
		} else if (g_line_len > 0) {
			write("\r\n");
			g_line[g_line_len] = '\0';
			handle_line(g_line);
			g_line_len = 0;
			prompt();
		}
		break;
	case '\b':
	case 0x7f:
		if (g_line_len > 0 && !g_discarding) {
			--g_line_len;
			write("\b \b");
		}
		break;
	case '\0':
		break;
	default:
		if (g_discarding) {
			break;
		}
		if ((g_line_len + 1u) < kLineMax) {
			g_line[g_line_len++] = static_cast<char>(b);
			write_raw(reinterpret_cast<const char *>(&b), 1);
		} else {
			g_discarding = true;
			g_line_len = 0;
			write("\r\n");
			err("line too long");
			prompt();
		}
		break;
	}
}

}

void on_uart_rx()
{
	(void)ring_push(g_rx_byte);
	arm_rx();
}

void on_uart_err()
{
	clear_rx_errors();
	arm_rx();
}

void say(const char *s)
{
	write(s);
	write("\r\n");
}

void init()
{
	g_rx_head = 0;
	g_rx_tail = 0;
	g_line_len = 0;
	g_discarding = false;
	g_snr_db = 99;
	g_level_db = 0;
	apply_level_snr();

	(void)HAL_UART_AbortReceive(&huart2);
	clear_rx_errors();
	arm_rx();

	write("BatTester ready\r\n");
	cmd_help();
	prompt();
}

void poll()
{
	clear_rx_errors();
	uint8_t b;
	while (ring_pop(b)) {
		consume_byte(b);
	}
}

}

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart2) {
		cli::on_uart_rx();
	}
}

extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart2) {
		cli::on_uart_err();
	}
}

#ifndef LOOP_CLI_HPP
#define LOOP_CLI_HPP

/* Line protocol of the loop image. Commands are words; every reply line starts
   with '@' (digitalSpine todo/fluxStationLoopSource.md §5.6). No echo. */
namespace cli {

void init();
void poll();

}

#endif

#include "smt2parser.h"
#include "smtlib_frontend.h"
#include"dl_cmds.h"
#include"dbg_cmds.h"
#include"polynomial_cmds.h"
#include"subpaving_cmds.h"
#include"smt_strategic_solver.h"
#include"smt_solver.h"

#include"memory_manager.h"
#include"env_params.h"
#include"gparams.h"
#include "symbol.h"
#include <string>

using namespace std;

// Must be here in order to compile thanks to bizzare bits
// with global variables in other files (yay C++!)
bool                g_display_statistics  = false;

const string FILE_OR_INPUT_DELIMETER = "---FINISHED---";

// Protocol:
// -Read line as SMT command
// -Execute the line, producing output
// -Read line.  If it's FILE_OR_INPUT_DELIMETER, then the file is done,
//  and we reset the context, etc.  Otherwise, we read the line
//  as an SMT command and repeat the process.

enum ReadLineResult {
  STREAM_COMPLETE,
  FILE_COMPLETE,
  MORE_TO_FILE
};

ReadLineResult read_line(cmd_context& context, istream& input) {
  string line;

  if (getline(input, line)) {
    if (line.compare(FILE_OR_INPUT_DELIMETER) == 0) {
      return FILE_COMPLETE;
    } else {
      process_smt_command(context, line);
      cout << endl << FILE_OR_INPUT_DELIMETER << endl;
      return MORE_TO_FILE;
    }
  } else {
    return STREAM_COMPLETE;
  }
}

// returns true if we have more files, else false
bool read_file_incrementally(istream& input) {
  cmd_context ctx;

  ctx.set_solver_factory(mk_smt_strategic_solver_factory());
  ctx.set_interpolating_solver_factory(mk_smt_solver_factory());

  install_dl_cmds(ctx);
  install_dbg_cmds(ctx);
  install_polynomial_cmds(ctx);
  install_subpaving_cmds(ctx);

  set_global_context(ctx);

  // loop until we hit a terminator of some sort
  while (true) {
    switch (read_line(ctx, input)) {
    case STREAM_COMPLETE:
      return false;
    case FILE_COMPLETE:
      return true;
    case MORE_TO_FILE:
      break;
    }
  }
}

void read_files(istream& input) {
  while (read_file_incrementally(input)) {}
}

int main(int argc, char** argv) {
  memory::initialize(0);
  memory::exit_when_out_of_memory(true, "ERROR: out of memory");

  // Handle division by zero soundly with respect to SMT-LIB
  gparams::set("rewriter.hi_div0", "false");
  gparams::set("old_simplify.bv.hi_div0", "false");
  
  env_params::updt_params();
  read_files(cin);
  return 0;
}

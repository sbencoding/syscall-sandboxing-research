#!/bin/bash

# Last action of the original loader script, separated from the compiling of llvm, so that they are separate steps when building the docker image
function check_exit_status {
  if [[ ! $? -eq 0 ]]; then
    echo -e "${RED}Error during last command, terminating${NC}"
    exit 1
  fi
}

echo -e "${GREEN}Compiling chestnut.o${NC}"
make -C ../libchestnut/
check_exit_status
cp ../libchestnut/chestnut.o llvm10/build/bin/

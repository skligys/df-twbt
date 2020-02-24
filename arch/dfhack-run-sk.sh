#!/bin/bash

# check for dwarffortress user directory
if [[ ! -d ~/.dwarffortress-sk ]] ; then
  mkdir -p ~/.dwarffortress-sk/data

  ln -s  /opt/dwarffortress-sk/raw       ~/.dwarffortress-sk/raw
  ln -s  /opt/dwarffortress-sk/libs      ~/.dwarffortress-sk/libs
  cp -rn /opt/dwarffortress-sk/data/init ~/.dwarffortress-sk/data/init

  for link in announcement art dipscript help index initial_movies movies shader.fs shader.vs sound speech ; do
    cp -r /opt/dwarffortress-sk/data/$link ~/.dwarffortress-sk/data/$link
  done
fi

# check for dfhack user directory
if [[ ! -d ~/.dwarffortress-sk/hack ]] ; then
  ln -s /opt/dwarffortress-sk/hack                ~/.dwarffortress-sk/hack
  ln -s /opt/dwarffortress-sk/stonesense          ~/.dwarffortress-sk/stonesense
  ln -s /opt/dwarffortress-sk/dfhack              ~/.dwarffortress-sk/dfhack
  ln -s /opt/dwarffortress-sk/dfhack-run          ~/.dwarffortress-sk/dfhack-run
  ln -s /opt/dwarffortress-sk/dfhack.init-example ~/.dwarffortress-sk/dfhack.init-example
  cp -r /opt/dwarffortress-sk/dfhack-config       ~/.dwarffortress-sk/dfhack-config

  cp ~/.dwarffortress-sk/dfhack.init{-example,}
fi

# workaround for bug in Debian/Ubuntu SDL patch
export SDL_DISABLE_LOCK_KEYS=1

cd ~/.dwarffortress-sk
exec ./dfhack-run-sk "$@"

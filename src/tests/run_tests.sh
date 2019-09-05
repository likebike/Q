#!/bin/bash
set -o errexit
set -o nounset

MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
Q_DIR="$MYDIR/.."

function setUp {
    echo Starting $1
    rm -r ~/.tmp/Q &>/dev/null || true
    return $?
}
function chk_qfile {
    [ -f "$HOME/.tmp/Q/$1" ]
    return $?
}

cd $Q_DIR

make


### NOTE, We need to execute all the tests in subshells or else the
### 'errexit' functionality does not apply.  (It does not apply to
### '&&' chains.  You can alternatively force proper behavior by
### adding a || false to the end of the chain.)

( setUp 'expect help' && ./Q 2>&1 | grep -q 'FIFO worker queue' )

( setUp '#1' && COMMAND=sleep ./Q 1                         && chk_qfile cmd_sleep.Q )
( setUp '#2' && COMMAND=/bin/sleep ./Q 1                    && chk_qfile cmd_sleep.Q )
( setUp '#3' && COMMAND=/bin/../var/../bin/sleep ./Q 1      && chk_qfile cmd_sleep.Q )
( setUp '#4' && COMMAND=/bin/../var/////../bin/sleep ./Q 1  && chk_qfile cmd_sleep.Q )
( setUp '#5' && COMMAND="$(python -c 'import os; print os.path.relpath("/bin/sleep")')" ./Q 1 && chk_qfile cmd_sleep.Q )
( setUp '#6' && COMMAND=sleep Q_FILE=~/.tmp/Q/pleas.Q ./Q 1  && chk_qfile pleas.Q )
( setUp '#7' && COMMAND=sleep Q_FILE=/ ./Q 1 2>/dev/null )          # If the lockfile can't be opened (like in the case of a directory), Q is designed to just run the COMMAND.
( setUp '#8' && touch ~/.tmp/Q && COMMAND=sleep Q_FILE=~/.tmp/Q ./Q 1 2>/dev/null )   # Intentionally break the Q_FILE dir structre by creating a file.  Q should succeed anyway, except without record locking.
( setUp '#9' && COMMAND=sleep Q_FILE=/etc/motd ./Q 1 2>/dev/null )  # Q never modifies any data on the filesystem -- it just locks with Record Locking.
( [ -f /etc/motd ] )  # ...Just checking...


(!( setUp 'Fail #1' && COMMAND=/bin/sleep/ ./Q 1 2>/dev/null      && chk_qfile cmd_sleep.Q ))
(!( setUp 'Fail #2' && COMMAND=/bin/sleep_sorry ./Q 1 2>/dev/null && chk_qfile cmd_sleep.Q ))

echo 'All tests successful.'

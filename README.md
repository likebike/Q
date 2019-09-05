# Q

A Process Queue.  See the website for a full description: http://q.likebike.org/

## Manpage

Here is Q's standard help text, for your easy reference:

    Q Version 1.1.1  --  a FIFO worker queue, capable of running multiple workers at once.

    Typical usage:  COMMAND=/usr/bin/rsync  ./Q  [args...]

    The args get passed directly to COMMAND.

    Environment Variables:

        COMMAND=path -- Required.  The path to the target executable.  Should usually be an absolute path.
        Q_FILE=path -- The path to the Q queue state file.  More info below.
        CONCURRENCY=num -- Controls the number of workers to run at once.  Default is 1.
        MAX_Q=num -- Controls the maximum queue size (# waiting to run).  Default is -1 (unlimited).
        AUTO_CREATE_DIRS -- Set to 0 to prevent the Q_FILE parent directories from being auto-created.
        VERBOSE -- Set to 1 to display verbose information.
        HELP -- Set to 1 to display this help message and then exit.

    By default, Q_FILE is set to ~/.tmp/Q/cmd_$(basename $COMMAND).Q, but you can change this.
    You can use the tilde (~) and environment variables ($VAR) in the Q_FILE.
    This enables you to achieve some interesting queuing structures:

        Q_FILE=example.Q                     # A relative path can queue jobs based on $PWD.
        Q_FILE=/tmp/shared.Q                 # A system-wide queue.  (You would need to chmod 666 it.)
        Q_FILE='~/myjobs.Q'                  # Use a per-account queue.
        Q_FILE='~/.tmp/Q/cgi_$SERVER_NAME.Q' # Assuming this is for CGI processes, use a per-domain queue.


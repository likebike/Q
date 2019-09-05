#!/bin/bash

## Here are some example .htaccess directives that you could use to install this:
## (It assumes that Q is located at ~/webapps/app/php54.cgi)
#
#  <FilesMatch ^clientIP_wrapper.sh$>
#      SetHandler cgi-script
#  </FilesMatch>
#  Action php54-q /clientIP_wrapper.sh
#  <FilesMatch \.php$>
#      SetHandler php54-q
#  </FilesMatch>

if [ -n "$REMOTE_ADDR" ]  &&  echo 127.0.0.1 $(hostname -I) | grep -qF "$REMOTE_ADDR"; then
    # Local Request.  Don't use the queue.
    exec /home/php-cgi/php54.cgi "$@"
fi

# Remote Request.  Use the queue.
exec $HOME/webapps/app/php54.cgi "$@"


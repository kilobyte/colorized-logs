#!/bin/sh

rm -f commands.h
rm -f load_commands.h
ARGS="_command(char *arg, struct session *ses)"
while read FUNC
    do
        if [ -z "$FUNC" ]
            then
                continue
        fi
        case $FUNC in
            \#*)    ;;
            \**)    FUNC=`echo "$FUNC"|sed 's/^\*//'`
                    echo "extern struct session *$FUNC$ARGS;" >>commands.h
                    echo "add_command(c_commands, \"$FUNC\", (t_command)${FUNC}_command);" >>load_commands.h
                    ;;
            *)      echo "extern void            $FUNC$ARGS;" >>commands.h
                    echo "add_command(commands, \"$FUNC\", ${FUNC}_command);" >>load_commands.h
                    ;;
        esac
    done

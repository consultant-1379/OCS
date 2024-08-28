#!/bin/bash
##
# ------------------------------------------------------------------------
#     Copyright (C) 2012 Ericsson AB. All rights reserved.
# ------------------------------------------------------------------------
##
# Name:
#       ipnaadm.sh
# Description:
#       A script to wrap the invocation of raidmgr from the COM CLI.
# Note:
#       None.
##
# Usage:
#       None.
##
# Output:
#       None.
##
# Changelog:
# - Tue Oct 01 2012 - Tuan Nguyen (xtuangu)
#       First version.
##

/usr/bin/sudo /opt/ap/ocs/bin/ipnaadm "$@"

exit $?


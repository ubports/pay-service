#!/bin/bash -e
# This small script sets up the variables needed for acceptance
# testing of the pay-service and pay related things.


SSO_AUTH_BASE_URL=https://login.staging.ubuntu.com
SSO_UONE_BASE_URL=https://staging.one.ubuntu.com
PAY_BASE_URL=https://sc.staging.ubuntu.com/api/2.0/click/
URL_PACKAGE_INFO=https://search.apps.staging.ubuntu.com/api/v1/package/
ACCOUNT_CREDS_URL=https://login.staging.ubuntu.com/api/v2/tokens/oauth
ADD_PAYMENT_URL=https://sc.staging.ubuntu.com/api/2.0/click/paymentmethods/add/
U1_SEARCH_BASE_URL=https://search.apps.staging.ubuntu.com/

echo "Setting up upstart environment variables"

/sbin/initctl set-env --global SSO_AUTH_BASE_URL=$SSO_AUTH_BASE_URL
/sbin/initctl set-env --global SSO_UONE_BASE_URL=$SSO_UONE_BASE_URL
/sbin/initctl set-env --global PAY_BASE_URL=$PAY_BASE_URL
/sbin/initctl set-env --global URL_PACKAGE_INFO=$URL_PACKAGE_INFO
/sbin/initctl set-env --global ACCOUNT_CREDS_URL=$ACCOUNT_CREDS_URL
/sbin/initctl set-env --global ADD_PAYMENT_URL=$ADD_PAYMENT_URL
/sbin/initctl set-env --global U1_SEARCH_BASE_URL=$U1_SEARCH_BASE_URL

echo "Setting up dbus environment variables"

gdbus call --session \
	--dest org.freedesktop.DBus \
	--object-path / \
	--method org.freedesktop.DBus.UpdateActivationEnvironment \
	"[{'SSO_AUTH_BASE_URL', '$SSO_AUTH_BASE_URL'}, {'SSO_UONE_BASE_URL', '$SSO_UONE_BASE_URL'}, {'PAY_BASE_URL', '$PAY_BASE_URL'}, {'URL_PACKAGE_INFO', '$URL_PACKAGE_INFO'}, {'ACCOUNT_CREDS_URL', '$ACCOUNT_CREDS_URL'}, {'ADD_PAYMENT_URL', '$ADD_PAYMENT_URL'}, {'U1_SEARCH_BASE_URL', '$U1_SEARCH_BASE_URL'}]"

echo "Restarting scope registry"
/sbin/restart scope-registry

echo "Restarting pay service"
/sbin/restart pay-service

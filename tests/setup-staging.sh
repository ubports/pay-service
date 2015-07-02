#!/bin/bash -e
# This small script sets up the variables needed for acceptance
# testing of the pay-service and pay related things.


SSO_AUTH_BASE_URL=https://login.staging.ubuntu.com
SSO_UONE_BASE_URL=https://staging.one.ubuntu.com
PAY_BASE_URL=https://myapps.developer.staging.ubuntu.com
URL_PACKAGE_INFO=https://search.apps.staging.ubuntu.com/api/v1/package/
ACCOUNT_CREDS_URL=https://login.staging.ubuntu.com/api/v2/tokens/oauth
U1_SEARCH_BASE_URL=https://search.apps.staging.ubuntu.com/
CLICK_STORE_ENABLE_PURCHASES=1

echo "Setting up upstart environment variables"

/sbin/initctl set-env --global SSO_AUTH_BASE_URL=$SSO_AUTH_BASE_URL
/sbin/initctl set-env --global SSO_UONE_BASE_URL=$SSO_UONE_BASE_URL
/sbin/initctl set-env --global PAY_BASE_URL=$PAY_BASE_URL
/sbin/initctl set-env --global URL_PACKAGE_INFO=$URL_PACKAGE_INFO
/sbin/initctl set-env --global ACCOUNT_CREDS_URL=$ACCOUNT_CREDS_URL
/sbin/initctl set-env --global U1_SEARCH_BASE_URL=$U1_SEARCH_BASE_URL
/sbin/initctl set-env --global CLICK_STORE_ENABLE_PURCHASES=$CLICK_STORE_ENABLE_PURCHASES

echo "Setting up dbus environment variables"

gdbus call --session \
    --dest org.freedesktop.DBus \
    --object-path / \
    --method org.freedesktop.DBus.UpdateActivationEnvironment \
    "[{'SSO_AUTH_BASE_URL', '$SSO_AUTH_BASE_URL'}, {'SSO_UONE_BASE_URL', '$SSO_UONE_BASE_URL'}, {'PAY_BASE_URL', '$PAY_BASE_URL'}, {'URL_PACKAGE_INFO', '$URL_PACKAGE_INFO'}, {'ACCOUNT_CREDS_URL', '$ACCOUNT_CREDS_URL'}, {'U1_SEARCH_BASE_URL', '$U1_SEARCH_BASE_URL'}, {'CLICK_STORE_ENABLE_PURCHASES', '$CLICK_STORE_ENABLE_PURCHASES'}]"

echo "Working around pay-ui Click hook"

PAY_UI_TRIPLET=`ubuntu-app-triplet com.canonical.payui`
if [ ! -z $PAY_UI_TRIPLET ] && [ ! -e ${HOME}/.cache/pay-service/pay-ui/${PAY_UI_TRIPLET}.desktop ] ; then
    mkdir -p ${HOME}/.cache/pay-service/pay-ui
    ln -s ${HOME}/.cache/ubuntu-app-launch/desktop/${PAY_UI_TRIPLET}.desktop ${HOME}/.cache/pay-service/pay-ui/
fi

echo "Restarting scope registry"
/sbin/restart scope-registry

echo "Restarting pay service"
/sbin/restart pay-service

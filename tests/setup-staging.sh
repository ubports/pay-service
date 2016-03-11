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

echo "Restarting scope registry"
/sbin/restart scope-registry

STAGING_KEY_ID=9D7FAC7F5DEEC972
STAGING_KEYRING_PATH=/usr/share/debsig/keyrings/${STAGING_KEY_ID}
STAGING_POLICY_PATH=/etc/debsig/policies/${STAGING_KEY_ID}

PROD_KEY_ID=608FF2D200A0A71F
PROD_POLICY_PATH=/etc/debsig/policies/${PROD_KEY_ID}

if [ ! -d ${STAGING_KEYRING_PATH} -o ! -d ${STAGING_POLICY_PATH} ]; then
    echo "Setting up staging GPG key"
    sudo mount -o remount,rw /
    sudo mkdir -p ${STAGING_KEYRING_PATH}
    sudo gpg --no-default-keyring \
        --keyring ${STAGING_KEYRING_PATH}/click-store.gpg \
        --keyserver keyserver.ubuntu.com --recv-keys ${STAGING_KEY_ID}
    sudo cp -rf ${PROD_POLICY_PATH} ${STAGING_POLICY_PATH}
    sudo perl -p -i -e "s/${PROD_KEY_ID}/${STAGING_KEY_ID}/g" \
        ${STAGING_POLICY_PATH}/generic.pol
    echo "Finished importing staging GPG key"
fi

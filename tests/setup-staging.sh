#!/bin/bash -e
# This small script sets up the variables needed for acceptance
# testing of the pay-service and pay related things.

echo "Setting up staging environment variables"

initctl set-env --global SSO_AUTH_BASE_URL=https://login.staging.ubuntu.com
initctl set-env --global SSO_UONE_BASE_URL=https://staging.one.ubuntu.com
initctl set-env --global PAY_BASE_URL=https://sc.staging.ubuntu.com/api/2.0/click/
initctl set-env --global URL_PACKAGE_INFO=https://search.apps.staging.ubuntu.com/api/v1/package/
initctl set-env --global ACCOUNT_CREDS_URL=https://login.staging.ubuntu.com/api/v2/tokens/oauth
initctl set-env --global ADD_PAYMENT_URL=https://sc.staging.ubuntu.com/api/2.0/click/paymentmethods/add/
initctl set-env --global U1_SEARCH_BASE_URL=https://search.apps.staging.ubuntu.com/

echo "Restarting scope registry"
restart scope-registry

echo "Restarting pay service"
restart pay-service

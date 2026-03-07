#!/bin/bash
set -euo pipefail

# リポジトリのルートディレクトリへ移動
cd "$(dirname "$0")/../.."

STORE_FILE="backend/basic-auth-users.local.json"
RETENTION_DAYS="${LOG_RETENTION_DAYS:-365}"

usage() {
    echo "Usage: ./scripts/aws/deploy-backend.sh [retention_days]"
    echo "Example: ./scripts/aws/deploy-backend.sh 30"
}

if ! command -v jq >/dev/null 2>&1; then
    echo "Error: jq is required."
    exit 1
fi

if [ "$#" -gt 1 ]; then
    usage
    exit 1
fi

if [ "$#" -eq 1 ]; then
    RETENTION_DAYS="$1"
fi

case "$RETENTION_DAYS" in
    ''|*[!0-9]*)
        echo "Error: retention_days must be a positive integer."
        exit 1
        ;;
esac

if [ "$RETENTION_DAYS" -le 0 ]; then
    echo "Error: retention_days must be greater than 0."
    exit 1
fi

if [ -f "$STORE_FILE" ]; then
    BASIC_AUTH_VALUE=$(jq -c '[.[].auth]' "$STORE_FILE")
else
    BASIC_AUTH_VALUE='[]'
fi

echo "Deploy parameters:"
echo "  BasicAuthValidAuths=$BASIC_AUTH_VALUE"
echo "  LogRetentionDays=$RETENTION_DAYS"

cd backend
sam build
sam deploy --parameter-overrides \
  BasicAuthValidAuths="$BASIC_AUTH_VALUE" \
  LogRetentionDays="$RETENTION_DAYS"

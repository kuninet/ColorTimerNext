#!/bin/bash
set -e

# リポジトリのルートディレクトリへ移動
cd "$(dirname "$0")/../.."

STORE_FILE="backend/basic-auth-users.local.json"

usage() {
    echo "Usage:"
    echo "  ./scripts/aws/manage-basic-auth.sh add <username> <password>"
    echo "  ./scripts/aws/manage-basic-auth.sh remove <username>"
    echo "  ./scripts/aws/manage-basic-auth.sh list"
    echo "  ./scripts/aws/manage-basic-auth.sh print-parameter"
}

if ! command -v jq >/dev/null 2>&1; then
    echo "Error: jq is required."
    exit 1
fi

ACTION=${1:-}

if [ -z "$ACTION" ]; then
    usage
    exit 1
fi

if [ ! -f "$STORE_FILE" ]; then
    printf '[]\n' > "$STORE_FILE"
fi

print_deploy_hint() {
    PARAM_VALUE=$(jq -c '[.[].auth]' "$STORE_FILE")
    echo "Saved credentials to $STORE_FILE (gitignored)."
    echo "Deploy with:"
    echo "  ./scripts/aws/deploy-backend.sh"
    echo "Or run manually:"
    echo "  cd backend"
    echo "  sam deploy --parameter-overrides BasicAuthValidAuths='$PARAM_VALUE' LogRetentionDays=\${LOG_RETENTION_DAYS:-365}"
}

if [ "$ACTION" == "add" ]; then
    if [ "$#" -ne 3 ]; then
        usage
        exit 1
    fi

    USER=$2
    PASS=$3
    AUTH_B64=$(printf "%s:%s" "$USER" "$PASS" | base64 | tr -d '\n')
    AUTH_STRING="Basic ${AUTH_B64}"
    TMP_FILE=$(mktemp)

    jq --arg user "$USER" --arg auth "$AUTH_STRING" '
      map(select(.username != $user)) + [{username: $user, auth: $auth}]
      | sort_by(.username)
    ' "$STORE_FILE" > "$TMP_FILE"
    mv "$TMP_FILE" "$STORE_FILE"

    echo "Added/updated user '$USER'."
    print_deploy_hint
elif [ "$ACTION" == "remove" ]; then
    if [ "$#" -ne 2 ]; then
        usage
        exit 1
    fi

    USER=$2
    TMP_FILE=$(mktemp)
    jq --arg user "$USER" 'map(select(.username != $user))' "$STORE_FILE" > "$TMP_FILE"
    mv "$TMP_FILE" "$STORE_FILE"

    echo "Removed user '$USER' if present."
    print_deploy_hint
elif [ "$ACTION" == "list" ]; then
    jq -r 'if length == 0 then "No users configured." else .[] | .username end' "$STORE_FILE"
elif [ "$ACTION" == "print-parameter" ]; then
    jq -c '[.[].auth]' "$STORE_FILE"
else
    echo "Invalid action: $ACTION"
    usage
    exit 1
fi

#!/bin/bash
set -e

# リポジトリのルートディレクトリへ移動
cd "$(dirname "$0")/../.."

if [ "$#" -ne 3 ]; then
    echo "Usage: ./scripts/aws/manage-basic-auth.sh [add|remove] <username> <password>"
    echo "Example: ./scripts/aws/manage-basic-auth.sh add user1 pass1"
    exit 1
fi

ACTION=$1
USER=$2
PASS=$3
# Mac/Linux両対応のbase64エンコード
AUTH_B64=$(printf "%s:%s" "$USER" "$PASS" | base64)
AUTH_STRING="\"Basic ${AUTH_B64}\""

TEMPLATE_FILE="backend/template.yaml"

if [ "$ACTION" == "add" ]; then
    echo "Adding/Updating user '$USER'..."
    # 既に存在する場合は無視する簡易的なチェック
    if grep -q "$AUTH_STRING" "$TEMPLATE_FILE"; then
        echo "User '$USER' is already configured."
    else
        # 簡易的に配列の最後の要素か、先頭に追加する (sed等で無理やり挿入)
        if [[ "$OSTYPE" == "darwin"* ]]; then
          sed -i '' -e "/var validAuths = \[/a\\
                $AUTH_STRING,
" "$TEMPLATE_FILE"
        else
          sed -i "/var validAuths = \[/a\                $AUTH_STRING," "$TEMPLATE_FILE"
        fi
        echo "User '$USER' added successfully."
    fi

elif [ "$ACTION" == "remove" ]; then
    echo "Removing user '$USER'..."
    if [[ "$OSTYPE" == "darwin"* ]]; then
      sed -i '' -e "/$AUTH_STRING/d" "$TEMPLATE_FILE"
    else
      sed -i "/$AUTH_STRING/d" "$TEMPLATE_FILE"
    fi
    echo "User '$USER' removed."
else
    echo "Invalid action. Use 'add' or 'remove'."
    exit 1
fi

echo "Credentials updated locally in backend/template.yaml."
echo "To apply changes to AWS, please deploy the backend stack:"
echo "  cd backend"
echo "  sam build"
echo "  sam deploy"

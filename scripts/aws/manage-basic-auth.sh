#!/bin/bash
set -e

# リポジトリのルートディレクトリへ移動
cd "$(dirname "$0")/../.."

if [ "$#" -ne 2 ]; then
    echo "Usage: ./scripts/aws/manage-basic-auth.sh <username> <password>"
    exit 1
fi

USER=$1
PASS=$2
# Mac/Linux両対応のbase64エンコード
AUTH_B64=$(printf "%s:%s" "$USER" "$PASS" | base64)

echo "Updating template.yaml with new credentials..."

# backend/template.yaml の中の expectedAuth をsedで直接書き換える
if [[ "$OSTYPE" == "darwin"* ]]; then
  # Mac
  sed -i '' "s/var expectedAuth = \"Basic .*\";/var expectedAuth = \"Basic ${AUTH_B64}\";/" backend/template.yaml
else
  # Linux
  sed -i "s/var expectedAuth = \"Basic .*\";/var expectedAuth = \"Basic ${AUTH_B64}\";/" backend/template.yaml
fi

echo "Credentials updated locally in backend/template.yaml."
echo "To apply changes to AWS, please deploy the backend stack:"
echo "  cd backend"
echo "  sam build"
echo "  sam deploy"

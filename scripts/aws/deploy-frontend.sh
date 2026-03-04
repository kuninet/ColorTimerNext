#!/bin/bash
set -e

STACK_NAME=${STACK_NAME:-colortimernext-prod}
REGION=${AWS_DEFAULT_REGION:-ap-northeast-1}

echo "Fetching CloudFormation outputs for stack: $STACK_NAME ..."
OUTPUTS=$(aws cloudformation describe-stacks --stack-name "$STACK_NAME" --region "$REGION" --query "Stacks[0].Outputs" --output json)

export S3_BUCKET=$(echo "$OUTPUTS" | jq -r '.[] | select(.OutputKey=="FrontendBucketName") | .OutputValue')
export CLOUDFRONT_DOMAIN=$(echo "$OUTPUTS" | jq -r '.[] | select(.OutputKey=="CloudFrontDomainName") | .OutputValue')

# CloudFront Distribution ID を ドメイン名から逆引き
export CLOUDFRONT_ID=$(aws cloudfront list-distributions --region "$REGION" --query "DistributionList.Items[?DomainName=='$CLOUDFRONT_DOMAIN'].Id | [0]" --output text)

if [ -z "$S3_BUCKET" ] || [ "$S3_BUCKET" == "null" ]; then
  echo "Error: FrontendBucketName not found in stack outputs. Is the backend stack deployed?"
  exit 1
fi

echo "S3 Bucket: $S3_BUCKET"
echo "CloudFront ID: $CLOUDFRONT_ID"

# フロントエンドのビルド
cd frontend
echo "Building React frontend..."
npm install
npm run build

# S3へ同期
echo "Syncing to S3..."
aws s3 sync dist/ s3://$S3_BUCKET/ --delete

# CloudFrontのキャッシュパージ
if [ -n "$CLOUDFRONT_ID" ] && [ "$CLOUDFRONT_ID" != "None" ] && [ "$CLOUDFRONT_ID" != "null" ]; then
  echo "Invalidating CloudFront cache..."
  aws cloudfront create-invalidation --distribution-id $CLOUDFRONT_ID --paths "/*"
else
  echo "CloudFront ID not found, skipping cache invalidation."
fi

echo "Frontend deployment complete!"

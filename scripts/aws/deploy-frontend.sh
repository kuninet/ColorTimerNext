#!/bin/bash
set -e

# SAMで出力したS3バケット名とCloudFrontディストリビューションIDを取得
cd backend
echo "Fetching AWS SAM outputs..."
export S3_BUCKET=$(sam list outputs --stack-name ColorTimerNext --region ap-northeast-1 --output json | jq -r '.[] | select(.OutputKey=="FrontendBucketName") | .OutputValue')
export CLOUDFRONT_ID=$(aws cloudfront list-distributions --query "DistributionList.Items[?Aliases.Items==null && Origins.Items[0].Id=='FrontendS3Origin'].Id | [0]" --output text)

if [ -z "$S3_BUCKET" ] || [ "$S3_BUCKET" == "null" ]; then
  echo "Error: FrontendBucketName not found in SAM outputs. Is the backend stack deployed?"
  exit 1
fi

echo "S3 Bucket: $S3_BUCKET"
echo "CloudFront ID: $CLOUDFRONT_ID"

# フロントエンドのビルド
cd ../frontend
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

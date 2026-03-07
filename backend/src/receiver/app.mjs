import { DynamoDBClient } from "@aws-sdk/client-dynamodb";
import { DynamoDBDocumentClient, PutCommand } from "@aws-sdk/lib-dynamodb";

const client = new DynamoDBClient({});
const docClient = DynamoDBDocumentClient.from(client);

// テーブル名は環境変数から取得
const tableName = process.env.TABLE_NAME;
const retentionDays = Number.parseInt(process.env.LOG_RETENTION_DAYS ?? "365", 10);
const normalizedRetentionDays = Number.isNaN(retentionDays) || retentionDays <= 0 ? 365 : retentionDays;
const retentionSeconds = normalizedRetentionDays * 24 * 60 * 60;

export const handler = async (event) => {
    console.log("Received event:", JSON.stringify(event));

    try {
        // リクエストボディのパース
        if (!event.body) {
            return buildResponse(400, { message: "Request body is missing" });
        }

        const body = JSON.parse(event.isBase64Encoded ? Buffer.from(event.body, 'base64').toString('utf-8') : event.body);
        const { device_id, action } = body;

        // バリデーション
        if (!device_id || !action || !['start', 'stop'].includes(action)) {
            return buildResponse(400, { message: "Invalid payload. 'device_id' and 'action' ('start' or 'stop') are required." });
        }

        // サーバー時刻（開始・終了時点のタイムスタンプ）
        const now = new Date();
        const timestamp = now.toISOString();
        const expireAt = Math.floor(now.getTime() / 1000) + retentionSeconds;

        const putCommand = new PutCommand({
            TableName: tableName,
            Item: {
                DeviceId: device_id,
                Timestamp: timestamp,
                Action: action,
                ExpireAt: expireAt,
            }
        });

        await docClient.send(putCommand);

        return buildResponse(200, {
            message: "Log recorded successfully",
            data: {
                device_id,
                action,
                timestamp,
                expire_at: expireAt
            }
        });

    } catch (err) {
        console.error("Error processing request:", err);
        return buildResponse(500, { message: "Internal Server Error" });
    }
};

const buildResponse = (statusCode, body) => {
    return {
        statusCode,
        headers: {
            "Content-Type": "application/json"
        },
        body: JSON.stringify(body)
    };
};

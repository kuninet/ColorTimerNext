import { DynamoDBClient } from "@aws-sdk/client-dynamodb";
import { DynamoDBDocumentClient, PutCommand } from "@aws-sdk/lib-dynamodb";

const client = new DynamoDBClient({});
const docClient = DynamoDBDocumentClient.from(client);

// テーブル名は環境変数から取得
const tableName = process.env.TABLE_NAME;

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
        if (!device_id || !action || !['start', 'end'].includes(action)) {
            return buildResponse(400, { message: "Invalid payload. 'device_id' and 'action' ('start' or 'end') are required." });
        }

        // サーバー時刻（開始・終了時点のタイムスタンプ）
        const timestamp = new Date().toISOString();

        // TTL の設定 (例として、30日後に消えるように設定する場合のコード例。環境変数または要件次第ですが、一旦コメントアウトで長めに設定可能)
        // DynamoDBのTTLはUNIXスタンプ(秒)
        const expireAt = Math.floor(Date.now() / 1000) + (60 * 60 * 24 * 30); // 30days

        const putCommand = new PutCommand({
            TableName: tableName,
            Item: {
                DeviceId: device_id,
                Timestamp: timestamp,
                Action: action,
                ExpireAt: expireAt
            }
        });

        await docClient.send(putCommand);

        return buildResponse(200, {
            message: "Log recorded successfully",
            data: {
                device_id,
                action,
                timestamp
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

import { DynamoDBClient } from "@aws-sdk/client-dynamodb";
import { DynamoDBDocumentClient, QueryCommand } from "@aws-sdk/lib-dynamodb";

const client = new DynamoDBClient({});
const docClient = DynamoDBDocumentClient.from(client);

const tableName = process.env.TABLE_NAME;

export const handler = async (event) => {
    console.log("Received event:", JSON.stringify(event));

    try {
        const queryParams = event.queryStringParameters || {};
        const deviceId = queryParams.device_id;
        const startDate = queryParams.start_date; // ex: 2026-03-01
        const endDate = queryParams.end_date;     // ex: 2026-03-31

        if (!deviceId) {
            return buildResponse(400, { message: "Missing required query parameter: 'device_id'" });
        }

        let commandInput = {
            TableName: tableName,
            KeyConditionExpression: "DeviceId = :deviceId",
            ExpressionAttributeValues: {
                ":deviceId": deviceId
            }
        };

        // 日付指定がある場合 (ソートキー Timestamp に対する BETWEEN もしくは >= <= 条件)
        if (startDate && endDate) {
            commandInput.KeyConditionExpression += " AND #ts BETWEEN :start AND :end";
            commandInput.ExpressionAttributeNames = {
                "#ts": "Timestamp"
            };
            // 終了日は時刻まで含めて 23:59:59.999Z とみなすなどの調整をフロントで行うか、APIで末尾にZを付与する等の工夫
            commandInput.ExpressionAttributeValues[":start"] = `${startDate}T00:00:00.000Z`;
            commandInput.ExpressionAttributeValues[":end"] = `${endDate}T23:59:59.999Z`;
        } else if (startDate) {
            commandInput.KeyConditionExpression += " AND #ts >= :start";
            commandInput.ExpressionAttributeNames = { "#ts": "Timestamp" };
            commandInput.ExpressionAttributeValues[":start"] = `${startDate}T00:00:00.000Z`;
        }

        const queryCommand = new QueryCommand(commandInput);
        const result = await docClient.send(queryCommand);

        return buildResponse(200, {
            items: result.Items || [],
            count: result.Count || 0
        });

    } catch (err) {
        console.error("Error processing query request:", err);
        return buildResponse(500, { message: "Internal Server Error" });
    }
};

const buildResponse = (statusCode, body) => {
    return {
        statusCode,
        headers: {
            "Content-Type": "application/json",
            // CORS対応: 複数オリジンがある場合はフロント側環境等へ限定するのも手
            "Access-Control-Allow-Origin": "*"
        },
        body: JSON.stringify(body)
    };
};

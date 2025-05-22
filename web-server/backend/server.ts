import mqtt from "mqtt";
import fs from "fs";
import path from "path";
import { v4 as uuidv4 } from "uuid";
import { MongoClient, GridFSBucket } from "mongodb";
import stream from "stream";
import express from "express";
import { MQTT_BROKER_URL, MQTT_TOPIC, IMAGE_SAVE_PATH, mongoUrl, dbName, port } from "./config";
import { Client } from "pg";



interface ImageMetadata {
    timestamp: string;
    deviceId: string;
    cameraParams: Record<string, any>;
    filename: string;
}
const app = express();
async function main() {
    if (!fs.existsSync(IMAGE_SAVE_PATH)) {
        fs.mkdirSync(IMAGE_SAVE_PATH, { recursive: true });
    }

    const pgClient = new Client({
        user: "user",
        host: "localhost",
        database: "image_db",
        password: "password",
        port: 5432,
    });
    await pgClient.connect();

    // Create table if not exists
    await pgClient.query(`
        CREATE TABLE IF NOT EXISTS images (
            id SERIAL PRIMARY KEY,
            filename TEXT,
            device_id TEXT,
            camera_params JSONB,
            timestamp TIMESTAMP
        )
    `);

    const client = mqtt.connect(MQTT_BROKER_URL);
    client.on("connect", () => {
        console.log("Connected to MQTT broker");
        client.subscribe(MQTT_TOPIC);
    });

    client.on("message", async (_topic, message) => {
        try {
            const { imageBase64, deviceId, cameraParams } = JSON.parse(message.toString());
            const buffer = Buffer.from(imageBase64, "base64");
            const filename = `${uuidv4()}.jpg`;
            const filePath = path.join(IMAGE_SAVE_PATH, filename);

            // Save image to filesystem
            await fs.promises.writeFile(filePath, buffer);

            // Save metadata to PostgreSQL
            await pgClient.query(
                `INSERT INTO images (filename, device_id, camera_params, timestamp)
                 VALUES ($1, $2, $3, $4)`,
                [filename, deviceId, cameraParams, new Date()]
            );

            console.log("Image saved:", filename);
        } catch (err) {
            console.error("Failed to process message:", err);
        }
    });

    app.get("/image/:filename", (req, res) => {
        const filename = req.params.filename;
        const filePath = path.join(IMAGE_SAVE_PATH, filename);

        if (fs.existsSync(filePath)) {
            res.sendFile(filePath);
        } else {
            res.status(404).send("Image not found");
        }
    });

    app.listen(port, () => {
        console.log(`Image server running at http://localhost:${port}`);
    });

}

main().catch(console.error);

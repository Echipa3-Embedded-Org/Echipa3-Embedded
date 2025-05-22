import mqtt from "mqtt";
import fs from "fs";
import path from "path";
import { v4 as uuidv4 } from "uuid";
import express from "express";
import { MQTT_BROKER_URL, MQTT_TOPIC, IMAGE_SAVE_PATH, port } from "./config";
import { Client } from "pg";

// Enhanced interface for metadata
interface ImageMetadata {
    deviceId: string;
    resolution: string;
    fps: number;
}



const app = express();

async function main() {
    // Ensure image directory exists
    fs.mkdirSync(IMAGE_SAVE_PATH, { recursive: true });

    // PostgreSQL setup
    const pgClient = new Client({
        user: "user",
        host: "localhost",
        database: "image_db",
        password: "password",
        port: 5432,
    });
    await pgClient.connect();

    await pgClient.query(`
        CREATE TABLE IF NOT EXISTS images (
            id SERIAL PRIMARY KEY,
            filename TEXT,
            device_id TEXT,
            resolution TEXT,
            fps INT,
            timestamp TIMESTAMP
        )
    `);

    // MQTT setup
    const client = mqtt.connect(MQTT_BROKER_URL);
    client.on("connect", () => {
        console.log("Connected to MQTT broker");
        client.subscribe("camera/images/#"); // Subscribe to hierarchical topics
    });

    client.on("message", async (topic, message) => {
        try {
            // Parse metadata from topic structure: camera/images/<deviceId>/<resolution>/<fps>
            const [, , deviceId, resolution, fps] = topic.split("/");

            if (!deviceId || !resolution || !fps) {
                throw new Error("Invalid topic format");
            }

            // Generate unique filename
            const filename = `${uuidv4()}.jpg`;
            const filePath = path.join(IMAGE_SAVE_PATH, filename);

            // Save raw binary directly to filesystem
            await fs.promises.writeFile(filePath, message);

            // Store metadata in PostgreSQL
            await pgClient.query(
                `INSERT INTO images (filename, device_id, resolution, fps, timestamp)
                 VALUES ($1, $2, $3, $4, $5)`,
                [filename, deviceId, resolution, parseInt(fps), new Date()]
            );

            console.log(`Saved ${filename} from ${deviceId}`);
        } catch (err) {
            console.error("Processing error:", err);
        }
    });

    // Image retrieval endpoint
    app.get("/image/:filename", (req, res) => {
        const filePath = path.join(IMAGE_SAVE_PATH, req.params.filename);
        fs.existsSync(filePath) ? res.sendFile(filePath) : res.status(404).send("Not found");
    });

    app.listen(port, () => {
        console.log(`Server running at http://localhost:${port}`);
    });
}

main().catch(console.error);

import React, { useState, useEffect } from "react";
import axios from "axios";

function App() {
  const [images, setImages] = useState([]);

  useEffect(() => {
    axios.get("http://localhost:8000/images").then((response) => {
      setImages(response.data.images);
    });
  }, []);

  return (
    <div>
      <h1>📷 Imagini primite de la ESP32-CAM</h1>
      <div style={{ display: "flex", flexWrap: "wrap" }}>
        {images.map((img) => (
          <div key={img.timestamp} style={{ margin: "10px" }}>
            <img src={img.url} alt="ESP32-CAM" width="200" />
            <p>{img.timestamp}</p>
          </div>
        ))}
      </div>
    </div>
  );
}

export default App;

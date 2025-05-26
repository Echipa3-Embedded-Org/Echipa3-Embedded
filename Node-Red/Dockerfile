FROM nodered/node-red:4.0.9

# Install required Node-RED packages
RUN npm install \
    node-red-dashboard \
    node-red-contrib-image-output \
    node-red-node-base64

COPY main-flow-config.json /data/flows.json
COPY settings.js /usr/src/node-red/node_modules/node-red/settings.js
# Reqiuired for Kubernetes
COPY main-flow-config.json /kubernetes/flows.json
COPY settings.js /kubernetes/settings.js

# Expose the default Node-RED port
EXPOSE 1880

CMD ["node-red"]

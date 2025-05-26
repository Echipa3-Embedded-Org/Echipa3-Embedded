FROM nodered/node-red:4.0.9

# Install required Node-RED packages
RUN npm install \
    node-red-dashboard \
    node-red-contrib-image-output \
    node-red-node-base64

COPY main-flow-config.json /data/flows.json
COPY main-flow-config.json /default-flows/flows.json

# Expose the default Node-RED port
EXPOSE 1880

CMD ["node-red"]
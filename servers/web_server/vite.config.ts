// vite.config.js
import { defineConfig } from "vite";
import * as YAML from "js-yaml";
import fs from "fs";
// import yaml file

const config_filepath = __dirname + "/../config.yml",
	config = (YAML.load(fs.readFileSync(config_filepath, "utf-8")) as any)
		?.WEB_SERVER,
	port = config?.port ?? "8080";

console.log(`Starting WEB server on port ${port}`);

export default defineConfig({
	server: {
		port,
	},
});

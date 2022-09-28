// Example route for the REST API
// The route path will be automatically generated from the file name
// And the directory structure
// E.g. this file will be available at /api/example since it is in the
// root of the routes/rest directory

const route = {
	type: "get", // Define the HTTP method
	// The callback function to be called when the route is accessed
	on: (req, res) => {
		console.log("hello");
		res.send("hello back");
	},
};

module.exports = route;

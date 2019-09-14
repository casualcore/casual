const path = require('path');

module.exports = {
    devServer: {
        port: 9000,
        proxy: {
            '/casual': {
                target: 'http://localhost:8080'
            }
        }
    }
};

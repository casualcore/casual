import api from '../apiservice';
import information from '../information/informationservice';

const scaleServer = async function (alias, instances) {

    try {

        const current = await information.getInstancesFromServer(alias);

        const scale = current + instances;
        if (scale > 0) {
            return await api.scaleServer(alias, scale);
        } else {
            return '';
        }
    } catch (error) {
        throw error;
    }
}


export default {
    scaleServer
}

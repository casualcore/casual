import axios, {AxiosInstance} from 'axios';


let axiosInstance: AxiosInstance;

const createAxiosInstance = function(): void {
  const baseUrl = process.env.VUE_APP_API_BASE ? process.env.VUE_APP_API_BASE : '';
  axiosInstance = axios.create({
    baseURL: baseUrl
  })
}

const http = function(): AxiosInstance {
  if(axiosInstance === undefined) {
    createAxiosInstance();
  }

  return axiosInstance;
}


export default http;
import http from "@/../http.config";

class AdminServiceImpl {

    async resetMetrics(service: string[] = []): Promise<string> {
        let reset = "";
        try {
            const input = {
                services: service
            }
            const response = await http().post("/api/v1/service/metric/reset", input);
            reset = response.data;


        } catch (error) {
            console.log(error)
        }
        return reset;
    }

}

export const AdminService = new AdminServiceImpl();
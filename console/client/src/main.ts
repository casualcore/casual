import { createApp } from "vue";
import App from "./App.vue";
import router from "./router";
import store from "./store";



import Loader from "@/components/global/Loader.vue";

createApp(App)
  .component("loading", Loader)
  .use(store)
  .use(router)
  .mount("#app");

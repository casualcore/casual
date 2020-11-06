import { createRouter, createWebHistory, RouteRecordRaw } from "vue-router";

const routes: Array<RouteRecordRaw> = [
  {
    path: "/",
    redirect: {
      name: "Console"
    }
  },
  {
    path: "/dashboard",
    name: "Console",
    component: () => import("@/views/Console.vue")
  },
  {
    path: "/groups",
    name: "Groups",
    component: () =>
      import(/* webpackChunkName: "about" */ "@/views/Groups.vue")
  },
  {
    path: "/servers",
    name: "Servers",
    component: () =>
      import(/* webpackChunkName: "about" */ "@/views/Servers.vue")
  },
  {
    path: "/executables",
    name: "Executables",
    component: () =>
      import(/* webpackChunkName: "about" */ "@/views/Executables.vue")
  },
  {
    path: "/services",
    name: "Services",
    component: () =>
      import(/* webpackChunkName: "about" */ "@/views/Services.vue")
  },
  {
    path: "/gateways",
    name: "Gateways",
    component: () =>
      import(/* webpackChunkName: "about" */ "@/views/Gateways.vue")
  },
  {
    path: "/transactions",
    name: "Transactions",
    component: () =>
      import(/* webpackChunkName: "about" */ "@/views/Transactions.vue")
  }
];

const router = createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes
});

export default router;

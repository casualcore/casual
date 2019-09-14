import Vue from 'vue';
import Router from 'vue-router';

import Casual from './components/Casual';
import CasualHome from './components/CasualHome';
import Groups from './components/Particles/Groups/Groups';
import Group from './components/Particles/Groups/Group';
import Servers from './components/Particles/Servers/Servers';
import Server from './components/Particles/Servers/Server';
import Services from './components/Particles/Services/Services';
import Service from './components/Particles/Services/Service';
import Executables from "./components/Particles/Servers/Executables/Executables";
import Executable from "./components/Particles/Servers/Executables/Executable";
import Gateways from "./components/Particles/Gateways/Gateways";
import Transactions from "./components/Particles/Transactions/Transactions";

Vue.use(Router);

const routes =
    [
        {
            path: '',
            redirect: '/console'
        },
        {
            path: '/console',
            component: Casual,
            children:
                [
                    {
                        path: '',
                        name: 'home',
                        component: CasualHome,
                        meta: {
                            title: 'Home'
                        }
                    },
                    {
                        path: 'groups',
                        redirect: 'groups/all'
                    },
                    {
                        path: 'groups/all',
                        name: 'groups',
                        component: Groups,
                        meta: {
                            title: 'Groups'
                        }
                    },
                    {
                        path: 'groups/group',
                        name: 'group',
                        component: Group,
                        meta: {
                            title: 'GROUP_NAME'
                        }
                    },
                    {
                        path: 'servers/all',
                        name: 'servers',
                        component: Servers,
                        meta: {
                            title: 'Servers'
                        }
                    },
                    {
                        path: 'servers/server',
                        name: 'server',
                        component: Server,
                        meta: {
                            title: 'SERVER_ALIAS'
                        }

                    },
                    {
                        path: 'services/all',
                        name: 'services',
                        component: Services,
                        meta: {
                            title: 'Services'
                        }
                    },
                    {
                        path: 'services/service',
                        name: 'service',
                        component: Service,
                        meta: {
                            title: 'SERVICE_NAME'
                        }
                    },
                    {
                        path: 'executables/all',
                        name: 'executables',
                        component: Executables,
                        meta: {
                            title: 'Executables'
                        }
                    },
                    {
                        path: 'executables/executable',
                        name: 'executable',
                        component: Executable,
                        meta: {
                            title: 'EXECUTABLE_ALIAS'
                        }
                    },
                    {
                        path: 'gateways/all',
                        name: 'gateways',
                        component: Gateways,
                        meta: {
                            title: 'Gateways'
                        }
                    },
                    {
                        path: 'transactions/all',
                        name: 'transactions',
                        component: Transactions,
                        meta: {
                            title: 'Transactions'
                        }
                    }

                ]
        },

    ]
const router = new Router({
    mode: 'history',
    routes
});

router.beforeEach(function (to, from, next) {
    let title = to.meta.title;
    switch (to.meta.title) {
        case 'GROUP_NAME':
        case 'SERVICE_NAME':
            title = to.query.name;
            break;
        case 'SERVER_ALIAS':
        case 'EXECUTABLE_ALIAS':
            title = to.query.alias;
            break;
    }

    document.title = title;
    next();
});


export default router;
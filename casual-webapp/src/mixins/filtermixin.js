import methods from './filter/methods';

export const filtermixin =
    {
        data() {
            return {}
        },
        methods,
        computed: {
            filterMix: function()
            {
                return "hello mix";
            }

        }
    }
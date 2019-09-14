<template>

    <div v-if="options.length > 0">

        <slot/>
        <select @change="filter" v-model="option">
            <option v-for="opt in options" :key="opt">{{opt}}</option>
        </select>
    </div>


</template>

<script>
    export default {
        name: "Filtering",
        props: ['filterOptions', 'type'],
        data() {
            return {
                option: ''
            }
        },
        computed: {
            options: function () {

                const options = [""];
                for (const opt of this.filterOptions) {
                    options.push(opt);
                }

                return options;
            }
        },
        methods: {
            filter: function () {
             //   console.log("filter", this.option);
                this.$emit('filterBy', this.option, this.type);
            },
            reset: function () {
                {
                    this.option = '';
                }

            }


        }

    }
</script>

<style scoped>

</style>
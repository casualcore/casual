<template>

    <div v-if="options.length > 0">

        <slot/>
        <select @change="sort" v-model="option">
            <option v-for="opt in options" :key="opt.name" :value="opt.value">{{opt.name}}</option>
        </select>
    </div>


</template>

<script>
    export default {
        name: "Sorting",
        props: ['sortingOptions'],
        data() {
            return {
                option: {}
            }
        },
        created() {

        },
        computed: {
            options: function () {

                return this.sortingOptions;
            },
        },
        methods: {
            reset: function () {
                this.option = this.sortingOptions[0];
                this.sort();
            },
            sort: function () {
                this.$emit('sortBy', this.option);
            },


        }

    }
</script>

<style scoped>

</style>
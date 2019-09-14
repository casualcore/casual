<template>
    <div>
        <button class="button button--simple" @click="go">{{value}}</button>
    </div>
</template>

<script>
    export default {
        name: "UpdateButton",
        props: ['value'],
        methods: {
            go: function () {
                this.$emit('go');
            }
        }
    }
</script>

<style scoped>

</style>
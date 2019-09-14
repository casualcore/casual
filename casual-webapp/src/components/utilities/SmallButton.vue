<template>
    <div>
        <button class="button button--small" @click="go"><slot></slot></button>
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
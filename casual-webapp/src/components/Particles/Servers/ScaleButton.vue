<template>
    <div>
        <button class="button button__scale" :class="arrow" @click="scale"></button>
    </div>
</template>

<script>
    export default {
        name: "ScaleButton",
        props: ['value', 'text'],
        computed: {
            arrow: function () {

                let classname = '';
                if (this.value > 0) {
                    classname = 'arrow-up';
                } else {
                    classname = 'arrow-down';

                }


                return {
                    [classname] : true
                };


            }
        },
        methods: {
            scale: function () {
                this.$emit('scale', this.value);
            }
        }
    }
</script>

<style scoped>

</style>